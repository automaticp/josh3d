#include "ResourceDatabase.hpp"
#include "ContainerUtils.hpp"
#include "Filesystem.hpp"
#include "Logging.hpp"
#include "ResourceType.hpp"
#include "RuntimeError.hpp"
#include "ThreadsafeQueue.hpp"
#include "UUID.hpp"
#include <algorithm>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/std.h>
#include <memory>
#include <stack>
#include <streambuf>


namespace josh {


namespace bip = boost::interprocess;


ResourceDatabase::ResourceDatabase(const Path& database_root)
    : database_root_ { canonical(database_root) }
    , table_filepath_{ database_root_ / "resources.jdb" }
{
    if (!is_directory(database_root_)) {
        throw error::RuntimeError(fmt::format("Specified database root \"{}\" is not an existing directory.", database_root_));
    }

    // This is subject to at least 2 filesystem races, but I have no idea how to avoid them.
    //
    // 1. The TOCTOU between exists() check and open() call. This one we ignore
    //    by opening a file in a non-destructive "append" mode. If a file was
    //    substituted, we won't do anything to it.
    //
    // 2. The potential of a file being deleted between creating the file and reopening it
    //    for non-destructive read/write access. We'll fail construction in this case
    //    as the filesystem is "too hostile".
    //
    // This is all likely complete nonsense to try to handle and I'm way too paranoid.

    if (!exists(table_filepath_)) {
        const auto mode = // ab
            std::ios::binary | std::ios::app;
        if (!table_filebuf_.open(table_filepath_, mode)) {
            throw error::RuntimeError(fmt::format("Cannot create database file \"{}\".", table_filepath_));
        }
        table_filebuf_.close();
    }

    const auto flags = // r+b
        std::ios::binary | std::ios::in | std::ios::out;

    if (!table_filebuf_.open(table_filepath_, flags)) {
        throw error::RuntimeError(fmt::format("Cannot open database file \"{}\".", table_filepath_));
    }

    // Get filesize. This is insane, but we need to skip mapping if the size is 0.
    const auto pos_beg  = table_filebuf_.pubseekoff(0, std::ios::beg);
    const auto pos_end  = table_filebuf_.pubseekoff(0, std::ios::end);
    const auto filesize = ptrdiff_t(pos_end - pos_beg);
    assert(filesize >= 0);

    file_mapping_ = bip::file_mapping(table_filepath_.c_str(), bip::read_write);

    if (filesize != 0) {
        mapped_file_  = bip::mapped_region(file_mapping_, bip::read_write);
        mapped_file_.advise(bip::mapped_region::advice_sequential);

        for (size_t row_id{ 0 }; row_id < num_rows(); ++row_id) {
            const Row&          row  = *row_ptr(row_id);
            const UUID&         uuid = row.uuid;
            const ResourcePath& path = row.filepath;
            if (uuid.is_nil()) {
                empty_rows_.emplace(row_id);
            } else {
                auto [it, was_emplaced] = table_.try_emplace(uuid, row_id);
                if (!was_emplaced) {
                    throw error::RuntimeError("TODO: Congrats, you got a duplicate UUID! No idea what to do with it yet.");
                }
                ++(path_uses_[path.view()]);
            }
        }

        mapped_file_.advise(bip::mapped_region::advice_random);
    }
}


auto ResourceDatabase::num_rows() const noexcept
    -> size_t
{
    return mapped_file_.get_size() / sizeof(Row);
}


auto ResourceDatabase::row_ptr(row_id row_id) const noexcept
    -> Row*
{
    assert(row_id < num_rows());
    std::byte* byte_ptr = (std::byte*)mapped_file_.get_address() + row_id * sizeof(Row);
    return std::launder(reinterpret_cast<Row*>(byte_ptr));
}


void ResourceDatabase::grow_file(size_t desired_num_rows) noexcept {
    if (desired_num_rows <= num_rows()) return;

    const size_t old_num_rows = num_rows();
    const size_t new_num_rows = desired_num_rows;

    // Resize the filebuf.
    const size_t new_size_bytes = new_num_rows * sizeof(Row);
    table_filebuf_.pubseekoff(ptrdiff_t(new_size_bytes - 1), std::ios::beg);
    table_filebuf_.sputc(0);
    table_filebuf_.pubsync(); // Sync in case file_mapping does not see this immediatelly.

    // Remap the file.
    mapped_file_ = bip::mapped_region(file_mapping_, bip::read_write);
    mapped_file_.advise(bip::mapped_region::advice_sequential);

    // Populate the empty rows. This is eww.
    for (size_t row_id{ old_num_rows }; row_id < new_num_rows; ++row_id) {
        empty_rows_.emplace(row_id);
    }
}


void ResourceDatabase::flush_row(row_id row_id) {
    const size_t offset_bytes = row_id * sizeof(Row);
    const size_t size_bytes   = sizeof(Row);
    mapped_file_.flush(offset_bytes, size_bytes);
}


void ResourceDatabase::bump_version() noexcept {
    ++state_version_;
}


void ResourceDatabase::new_entry(
    const UUID&         uuid,
    ResourceType        type,
    const ResourcePath& path,
    uint64_t            offset_bytes,
    uint64_t            size_bytes)
{
    // Expand the file if no empty rows are left. Use amortized allocation.
    // The fact that we use memory mapping sort of forces us to treat it like a memory alloc.
    if (empty_rows_.empty()) {
        const double growth_factor = 1.3;
        // NOTE: Adding one so that if num_rows() is 0 we are not screwed.
        const size_t desired_num_rows = 1 + size_t(double(num_rows()) * growth_factor);
        grow_file(desired_num_rows);
    }
    assert(!empty_rows_.empty());

    // Grab the first empty row, to fill the gaps from the beginning.
    const auto   it            = empty_rows_.begin();
    const row_id target_row_id = *it;

    *row_ptr(target_row_id) = {
        .uuid          = uuid,
        .type          = type,
        .filepath      = path,
        .offset_bytes  = offset_bytes,
        .size_bytes    = size_bytes,
    };

    empty_rows_.erase(it);
    table_.emplace(uuid, target_row_id);
    ++(path_uses_[path.view()]);

    flush_row(target_row_id);
}


auto ResourceDatabase::locate(const UUID& uuid) const noexcept
    -> ResourceLocation
{
    if (const auto* entry = try_find(table_, uuid)) {
        const row_id row_id = entry->second;
        const Row&   row    = *row_ptr(row_id);
        return {
            .file         = row.filepath.view(),
            .offset_bytes = row.offset_bytes,
            .size_bytes   = row.size_bytes,
        };
    } else {
        return {};
    }
}


void ResourceDatabase::update() {
    thread_local std::vector<UUID> remove_list; remove_list.clear();
    remove_queue_.lock_and([&](std::queue<UUID>& queue) {
        while (!queue.empty()) {
            // Move the data into a local list first.
            // try_remove_resource() could take a while,
            // don't want to hold the lock during that.
            remove_list.emplace_back(pop(queue));
        }
    });
    for (const UUID& uuid : remove_list) {
        try_remove_resource(uuid);
    }
}


namespace {


// This should not fail. Except when the preconditions are violated, of course.
auto path_from_hint(const ResourcePathHint& path_hint, size_t version) noexcept
    -> ResourcePath
{
    auto directory = path_hint.directory;
    auto name      = path_hint.name;
    auto extension = path_hint.extension;

    assert(directory.length() <= 64);
    assert(extension.length() <= 8);

    const bool   append_version = version != 0;
    const size_t version_length = append_version ? 4 : 0; // ".001", ".002", etc.

    // 2 extra bytes for "/" and ".".
    const size_t taken_length        = 2 + directory.length() + version_length + extension.length() ;
    const size_t allowed_name_length = ResourcePath::max_length - taken_length;

    ResourcePath result;
    char* ptr = result.filepath;

    // Write directory.
    ptr = std::ranges::copy(directory, ptr).out;
    *(ptr++) = Path::preferred_separator;

    // Write name.
    if (name.length() <= allowed_name_length) {
        // Life is simple.
        ptr = std::ranges::copy(name, ptr).out;
    } else {
        // Life is hard.
        auto truncated_name = name.substr(0, allowed_name_length);
        ptr = std::ranges::copy(truncated_name, ptr).out;
    }
    const size_t remaining_length = ResourcePath::max_length - (ptr - result.filepath);
    assert(remaining_length >= extension.length() + version_length);

    // Write version (if needed).
    if (append_version) {
        ptr = fmt::format_to_n(ptr, 4, ".{:03d}", version).out;
    }

    // Write extension.
    *(ptr++) = '.';
    ptr = std::ranges::copy(extension, ptr).out;

    result.length = ptr - result.filepath;
    return result;
}


} // namespace


auto ResourceDatabase::generate_resource(
    ResourceType            type,
    const ResourcePathHint& path_hint,
    size_t                  size_bytes)
        -> GeneratedResource
{
    assert(size_bytes > 0);

    // 1. Generate a unique UUID.
    UUID uuid;
    do {
        uuid = generate_uuid();
    } while (table_.contains(uuid));

    // 2. Create a valid unique path from the hint.
    // 3. Create and map a resource file of the required size.
    struct FILECloser { void operator()(std::FILE* f) const noexcept { std::fclose(f); } };
    using file_ptr = std::unique_ptr<std::FILE, FILECloser>;
    using bip::file_mapping, bip::mapped_region;

    ResourcePath  path;
    file_ptr      file;
    file_mapping  fmapping;
    mapped_region mregion;
    size_t        version       = 0;
    const size_t  version_limit = 1000; // We'll try a fixed number of times, then give up and throw.

    // NOTE: We are trying to be very gentle when it comes to creation
    // of a file here. No truncation is allowed, no existing files
    // should be overriden.
    //
    // There is still likely a way for a race to happen, particularly
    // when file mapping is reopened again from the same path.
    // I would consider this to be a defect in the boost API, that
    // the file_mapping does not have a constructor that takes a FILE*,
    // even though both POSIX and Windows support some form of fileno().
    //
    // NOTE: We can write our own file_mapping, if we so desire, but
    // no time for that right now.
    //
    // That said, if someone else just deletes the file after creation,
    // that would also constitute a violation of the invariant state, so
    // it's best if we consider a way to recover from that later instead.

    do {
        path = path_from_hint(path_hint, version++);

        if (path_uses_.contains(path.view())) {
            logstream() << fmt::format("[INFO]: Path \"{}\" is already in the database table. Retrying.\n", path.view());
            continue;
        }

        const Path full_path     = root() / path.view();
        const Path dst_directory = full_path.parent_path();
        std::filesystem::create_directories(dst_directory);


        file.reset(std::fopen(full_path.c_str(), "wbx"));
        if (!file) {
            logstream() << fmt::format("[INFO]: Could not open \"{}\" in exclusive mode. Retrying.\n", full_path);
            continue;
        }

        // Resize the file and flush.
        // Uhh, why is the argument `long`? RIP files above 2GB on Windows, I guess...
        // This is the case where std::filebuf works better. Damn it.
        const auto file_off = ptrdiff_t(size_bytes - 1);
        if (std::fseek(file.get(), file_off, SEEK_SET)) {
            logstream() << fmt::format("[INFO]: Could not seek to the file offset {} in \"{}\". Retrying.\n", file_off, full_path);
            file.reset();
            continue;
        }

        if (std::fputc(0, file.get()) == EOF) {
            logstream() << fmt::format("[INFO]: Could not write to the file offset {} in \"{}\". Retrying.\n", file_off, full_path);
            file.reset();
            continue;
        }

        if (std::fflush(file.get()) == EOF) {
            logstream() << fmt::format("[INFO]: Could not flush the file \"{}\". Retrying.\n", full_path);
            continue;
        }

        try {
            fmapping = file_mapping(full_path.c_str(), bip::read_write);
        } catch (const bip::interprocess_exception& e) {
            logstream() << fmt::format("[INFO]: Could not reopen file mapping for file \"{}\". Reason: \"{}\". Retrying.\n", full_path, e.what());
            file.reset();
            continue;
        }

        try {
            mregion = mapped_region(fmapping, bip::read_write);
        } catch (const bip::interprocess_exception& e) {
            logstream() << fmt::format("[INFO]: Could not map file \"{}\". Reason: \"{}\". Retrying.\n", full_path, e.what());
            file.reset();
            fmapping = {};
            continue;
        }

        if (mregion.get_size() != size_bytes) {
            // NOTE: This could be the result of the filesystem race where a file created by fopen
            // would be overwritten by another file before it is opened again with file_mapping.
            // Wouldn't have to think about it if the file_mapping just did it's platform-dependant
            // fileno() on the FILE pointer and we'd all live happily ever after.
            logstream() << fmt::format("[INFO]: Mapped file \"{}\" has unexpected size. Retrying.\n", full_path);
            file.reset();
            fmapping = {};
            continue;
        }

        break;
    } while (version < version_limit);

    if (version >= version_limit) {
        throw error::RuntimeError(fmt::format(
            "Too many attempts to create a file in the directory \"{}\" with name \"{}\" and extension \"{}\".",
            path_hint.directory, path_hint.name, path_hint.extension
        ));
    }

    new_entry(uuid, type, path, 0, size_bytes);
    bump_version();

    return {
        .uuid    = uuid,
        .mregion = MOVE(mregion)
    };
}


auto ResourceDatabase::try_unlink_record_(const UUID& uuid)
    -> UnlinkResult
{
    UnlinkResult result{
        .success             = false,
        .real_path           = {},
        .remaining_path_uses = {},
    };

    auto it = table_.find(uuid);

    if (it == table_.end()) return result;

    const row_id row_id = it->second;
    const Row*   row    = row_ptr(row_id);
    table_.erase(it);
    {
        auto db_path = row->filepath.view();
        auto it = path_uses_.find(db_path);
        assert(it != path_uses_.end());

        result.success             = true;
        result.real_path           = root() / db_path;
        result.remaining_path_uses = --(it->second); // Decrements.

        if (result.remaining_path_uses == 0) {
            path_uses_.erase(it);
        }
    }
    empty_rows_.emplace(row_id);

    std::memset(row_ptr(row_id), 0, sizeof(Row));
    flush_row(row_id);
    bump_version();

    return result;
}


auto ResourceDatabase::try_unlink_record(const UUID& uuid)
    -> bool
{
    return try_unlink_record_(uuid).success;
}


auto ResourceDatabase::try_remove_resource(const UUID& uuid)
    -> RemoveResourceOutcome
{
    UnlinkResult unlink_result = try_unlink_record_(uuid);
    if (!unlink_result.success) {
        return RemoveResourceOutcome::UUIDNotFound;
    }

    if (unlink_result.remaining_path_uses != 0) {
        return RemoveResourceOutcome::FileKept;
    }

    // Otherwise, try to nuke the file.
    if (std::filesystem::remove(unlink_result.real_path)) {
        return RemoveResourceOutcome::Success;
    } else {
        return RemoveResourceOutcome::FileNotFound;
    }
}


void ResourceDatabase::remove_resource_later(const UUID& uuid) {
    remove_queue_.push(uuid);
}


} // namespace josh
