#pragma once
#include "Filesystem.hpp"
#include "ResourceFiles.hpp"
#include "StringHash.hpp"
#include "ThreadsafeQueue.hpp"
#include "UUID.hpp"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_map_fwd.hpp>
#include <cstdint>
#include <fstream>
#include <functional>
#include <ranges>
#include <streambuf>
#include <string_view>


namespace josh {


struct ResourceLocation {
    std::string_view file;
    size_t           offset_bytes;
    size_t           size_bytes;
    explicit operator bool() const noexcept { return !file.empty(); }
};


struct ResourcePath {
    static constexpr size_t max_length = 95;

    uint8_t length;
    char    filepath[max_length];

    auto view() const noexcept -> std::string_view { assert(length <= max_length); return { filepath, length }; }
    operator std::string_view() const noexcept { return view(); }
};


struct ResourcePathHint {
    std::string_view directory;
    std::string_view name;
    std::string_view extension;
};


/*
This class controls a central resource database that consists of:

    1. a UUID <-> File+Offset table
    2. a set of resource files themselves

for a given resource root.

The table is a binary file with fixed-width rows describing
a relationship between an asset's UUID and the location on
the filesystem. The paths are always relative to the
directory where the table file is contained.
*/
class ResourceDatabase {
public:
    ResourceDatabase(const Path& database_root_dir);

    // Must be periodically called from the main thread.
    void update();

    // TODO: If `file` *is* a view, then what about relocating the entries under the hood?
    auto locate(const UUID& uuid) const noexcept -> ResourceLocation;

    // Returns a view of all UUIDs currently in the database.
    auto entries() const noexcept;

    struct GeneratedResource {
        UUID          uuid;
        mapped_region mregion;
    };

    // Creates a new resource in the database, in particular:
    //
    //   - Generates a *unique* UUID that does not currently exist in the database;
    //   - Creates a valid unique path from the supplied path hint;
    //   - Creates and maps a resource file of the required size;
    //   - Records an entry in the database table.
    //
    // Returns the generated UUID and a mapped_region of the newly created file.
    //
    // Path hint has the following requirements:
    //
    //   - `directory` must be 70 bytes long at max and should be specified relative to the database root.
    //   - `extension` must be 8 bytes long at max and should not include the period ".".
    //   - `name` will be truncated if too long, and a version suffix will be appended if not unique.
    //
    [[nodiscard]]
    auto generate_resource(
        const ResourcePathHint& path_hint,
        size_t                  size_bytes)
            -> GeneratedResource;

    // Attempts to unlink the database from the resource file.
    // Effectively removes the entry in the database table,
    // but does not remove the referenced file itself.
    //
    // Returns true on success, false if no such uuid in the database.
    bool try_unlink_record(const UUID& uuid);

    enum class RemoveResourceOutcome {
        Success      = 0, // Record unlinked, and the file is removed.
        FileKept     = 1, // Record unlinked, but the file is not removed due to it being used by other resources.
        FileNotFound = 2, // Record unlinked, but the file to remove was not found.
        UUIDNotFound = 3, // No such UUID in the database. Nothing is done.
    };

    // Attempts to remove the resource from the database.
    // Effectively removes *both* the entiry in the database table,
    // and the referenced resource file itself, if the entry is the
    // only user of the file.
    //
    // Returns an enum describing the result.
    auto try_remove_resource(const UUID& uuid)
        -> RemoveResourceOutcome;

    // Schedule the resource for removal later, during the update().
    // This is safe to use from any thread, and is the recommended
    // way to dispose of resources that failed construction for any reason.
    //
    // If UUID is not in the database, nothing is done, request is discarded.
    void remove_resource_later(const UUID& uuid);

    // Get the root path of the database. Each database resides in one unique root.
    auto root() const noexcept -> const Path& { return database_root_; }

    // A hint for caching the database table state. Every database update
    // changes the state version. If you cache info about the database
    // you can compare your last recorded version against the current one
    // to decide if the cache needs to be invalidated.
    //
    // Note that this only tracks state changes of the resource table,
    // not the contents of the resource files.
    auto state_version() const noexcept -> uint64_t { return state_version_; }


private:
    Path                               database_root_;
    Path                               table_filepath_;
    std::filebuf                       table_filebuf_;  // Keep open to be able to resize the file.
    boost::interprocess::file_mapping  file_mapping_;   // To quickly remap the file.
    boost::interprocess::mapped_region mapped_file_;    // Read/write to file through this.

    using row_id = size_t;

    boost::unordered_flat_map<UUID, row_id> table_;      // TODO: bimap?
    std::set<row_id>                        empty_rows_; // Intentionally ordered. TODO: There's a more efficient way to store this.
    boost::unordered_flat_map<std::string, size_t, string_hash, std::equal_to<>>
                                            path_uses_;  // Path -> Use Count. Use strings, not views, so that reallocation and reordering would not invalidate this.
    uint64_t                                state_version_{};

    ThreadsafeQueue<UUID>                   remove_queue_;  // To let other threads "cancel" failed resource imports.

    /*
    A single row in the table.
    */
    struct Row {
        UUID         uuid;         // UUID of the resource.
        ResourcePath filepath;     // Path to the resource relative to the database root.
        uint64_t     offset_bytes; // Offset of the resource data in the file.
        uint64_t     size_bytes;   // Size of the resource data in the file.
    };

    auto row_ptr(row_id row_id) const noexcept -> Row*;
    auto num_rows() const noexcept -> size_t;
    void grow_file(size_t desired_num_rows) noexcept;
    void flush_row(row_id row_id);
    void bump_version() noexcept;

    // Create a new entry, possibly resizing the table. No checks are made. Version is not updated.
    void new_entry(const UUID& uuid, const ResourcePath& path, uint64_t offset_bytes, uint64_t size_bytes);

    struct UnlinkResult {
        bool   success;             // True if unliked, false if no such UUID.
        Path   real_path;           // Full path to the file on disk. Empty if `!success`.
        size_t remaining_path_uses; // If 0 the file can be removed.
    };

    // Remove a record from the database table.
    auto try_unlink_record_(const UUID& uuid)
        -> UnlinkResult;

};


inline auto ResourceDatabase::entries() const noexcept {
    return table_ | std::views::keys;
}


} // namespace josh
