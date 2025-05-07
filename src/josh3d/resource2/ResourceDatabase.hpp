#pragma once
#include "Common.hpp"
#include "FileMapping.hpp"
#include "Filesystem.hpp"
#include "Resource.hpp"
#include "StringHash.hpp"
#include "ThreadsafeQueue.hpp"
#include "UUID.hpp"
#include <cstdint>
#include <functional>
#include <fstream>
#include <ranges>
#include <shared_mutex>
#include <streambuf>


namespace josh {


struct ResourcePath {
    static constexpr size_t max_length = 91;

    uint8_t length;
    char    path[max_length];

    auto view() const noexcept -> StrView { assert(length <= max_length); return { path, length }; }
    operator StrView() const noexcept { return view(); }
};


struct ResourcePathHint {
    StrView directory;
    StrView name;
    StrView extension;
};


struct ResourceLocation {
    ResourcePath file; // Filepath relative to the database root.
    size_t       offset_bytes;
    size_t       size_bytes;
    explicit operator bool() const noexcept { return file.length; }
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


Pattern of the .jdb table (ImHex):

const u64 max_filepath_size = 91;

struct Row {
    u8      uuid[16];                // UUID of the resource.
    u32     resource_type;           // Resource type.
    u8      filepath_size;           // Byte size of the string in the `filepath` field.
    char    filepath[filepath_size]; // Path to the resource relative to the database root.
    padding [max_filepath_size - filepath_size];
    u64     offset_bytes;            // Offset of the resource data in the file.
    u64     size_bytes;              // Size of the resource data in the file.
};

Row rows[sizeof($)/128] @ 0x0;
*/
class ResourceDatabase {
public:
    ResourceDatabase(const Path& database_root_dir);

    /*
    A single row in the table.
    */
    struct Row {
        UUID         uuid;         // UUID of the resource.
        ResourceType type;         // Type of the resource.
        ResourcePath filepath;     // Path to the resource relative to the database root.
        uint64_t     offset_bytes; // Offset of the resource data in the file.
        uint64_t     size_bytes;   // Size of the resource data in the file.
    };

    // Must be periodically called from the main thread.
    void update();

    auto locate(const UUID& uuid) const
        -> ResourceLocation;

    auto type_of(const UUID& uuid) const
        -> ResourceType;

    // Iterates all rows of the database table under a read lock.
    // No guarantees are given w.r.t. to the order of iteration.
    //
    // Calling any of the pulic interface functions of the database
    // inside `f` will deadlock the mutex. Don't do it.
    //
    // FIXME: This is a problematic way to expose this. There are
    // alternative interfaces, all with their respective tradeoffs.
    // Think about this a bit later.
    void for_each_row(std::invocable<const Row&> auto&& f) const;

    // Opens a mapping to the resource with the specified uuid.
    // Will return an empty mapping if the specified resource does not exist.
    [[nodiscard]]
    auto try_map_resource(const UUID& uuid)
        -> MappedRegion;

    // Opens a mapping to the resource with the specified uuid.
    // Will throw RuntimeError if the specified resource does not exist.
    [[nodiscard]]
    auto map_resource(const UUID& uuid)
        -> MappedRegion;

    struct GeneratedResource {
        UUID         uuid;
        MappedRegion mregion;
    };

    // Creates a new resource in the database, in particular:
    //
    //   - Generates a *unique* UUID that does not currently exist in the database;
    //   - Creates a valid unique path from the supplied path hint;
    //   - Creates and maps a resource file of the required size;
    //   - Records an entry in the database table.
    //
    // Returns the generated UUID and a MappedRegion of the newly created file.
    //
    // Path hint has the following requirements:
    //
    //   - `directory` must be 64 bytes long at max and should be specified relative to the database root.
    //   - `extension` must be 8 bytes long at max and should not include the period ".".
    //   - `name` will be truncated if too long, and a version suffix will be appended if not unique.
    //
    [[nodiscard]]
    auto generate_resource(
        ResourceType            type,
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
    [[deprecated("TOCTOU vulnerable without a lock. And completely useless.")]]
    auto state_version() const noexcept -> uint64_t;


private:
    Path                               database_root_;
    Path                               table_filepath_;

    std::filebuf                       table_filebuf_;  // Keep open to be able to resize the file.
    FileMapping                        file_mapping_;   // To quickly remap the file.
    MappedRegion                       mapped_file_;    // Read/write to file through this.

    using row_id = size_t;

    // Primary map of the database that helps locate all relevant info by a UUID.
    // TODO: bimap?
    HashMap<UUID, row_id>                   table_;

    // Intentionally ordered. TODO: There's a more efficient way to store this.
    OrderedSet<row_id>                      empty_rows_;

    // Map: Path -> Use Count. To only delete a file when there are no more users of it.
    // Use strings as keys, not views, so that reallocation and reordering would not invalidate this.
    HashMap<std::string, size_t, string_hash, std::equal_to<>>
                                            path_uses_;

    // Integer that represents database state. Every update increments the state version.
    uint64_t                                state_version_{};

    // Mutex of the whole database state above. Most operations are reads, contention is low.
    // Private functions (beginning with an "_") never lock the mutex.
    mutable std::shared_mutex               state_mutex_;

    // To let multiple threads "cancel" failed resource imports, without contending for the main state mutex.
    ThreadsafeQueue<UUID>                   remove_queue_;
    Vector<UUID>                            remove_list_; // Local remove list to not stall the remove queue.

    auto _row_ptr(row_id row_id) const noexcept -> Row*;
    auto _num_rows() const noexcept -> size_t;
    void _grow_file(size_t desired_num_rows) noexcept;
    void _flush_row(row_id row_id);
    void _bump_version() noexcept;

    // Create a new entry, possibly resizing the table. No checks are made. Version is not updated.
    void _new_entry(const UUID& uuid, ResourceType type, const ResourcePath& path, uint64_t offset_bytes, uint64_t size_bytes);

    struct UnlinkResult {
        bool   success;             // True if unliked, false if no such UUID.
        Path   real_path;           // Full path to the file on disk. Empty if `!success`.
        size_t remaining_path_uses; // If 0 the file can be removed.
    };

    // Remove a record from the database table.
    auto _try_unlink_record(const UUID& uuid) -> UnlinkResult;

    // Remove a record and the asociated file if last user.
    auto _try_remove_resource(const UUID& uuid) -> RemoveResourceOutcome;

};




inline void ResourceDatabase::for_each_row(
    std::invocable<const Row &> auto&& f) const
{
    const auto rlock = std::shared_lock(state_mutex_);
    for (const row_id row_id : table_ | std::views::values) {
        f(std::as_const(*_row_ptr(row_id)));
    }
}


} // namespace josh
