#pragma once
#include "AsyncCradle.hpp"
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "CommonConcepts.hpp"
#include "MeshRegistry.hpp"
#include "Resource.hpp"
#include "ContainerUtils.hpp"
#include "Coroutines.hpp"
#include "MutexPool.hpp"
#include "ResourceDatabase.hpp"
#include "TaskCounterGuard.hpp"
#include "UUID.hpp"
#include "UniqueFunction.hpp"
#include <boost/any/unique_any.hpp>
#include <cassert>
#include <cstdint>
#include <coroutine>
#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>


/*
Resource types are "intermediate" representations of each resource.

These are used for caching (retaining) the resource in its loaded
state separate from either its representation on-disk or in-scene.

TODO: Do we need this or could we just have a prefab registry?

TODO: List all use-cases and possible actions.

Stuff that needs to be supported:

    - Incremental streaming per-resource
        - Only data-intensive or all?
    - Fast query of resource availability:
        - Cold, Pending or Hot.
        - try_get_resource() API?
    - Reloading of referenced scene objects from changed resources.
        - Backreferencing from scene.
    - Reloading of resources from changed files, triggering above.
      (aka. hot-reloading).
    - Serializing the resource state back to files.
        - Overwriting or creating new.
    - Ability to "copy" a resource
        - For overriding or creating a modification
    - Incremental loading of scene information
    - Eviction system
        - Tracking of use-counts and load-times and other stats
        - Periodical clean-up of resources that are no longer in use
        - Eviction hints: "Evict asap", "Keep longer", "Never evict", etc...


Ordered by performance and latency requirement:

    - Availability queries (instant)
    - Lookup of resources  (instant when no writers)

    - Loading of scene info (minimum-blocking)
        - This is where central registry has a major disadvantage.
          Registry needs a mutex for *every* operation, whereas
          an independent table needs only to lock the table, not
          the resource.
    - Incremental streaming per-resource (can block on scene registry)

    - Eviction system (occasional runs on parts of the storage)
        - Shitty GC

    - Serializing the resource back to files.
    - Ability to copy the resource.
    - Reimporting of resoures from changed files.
        - Resource invalidation


*/
namespace josh {


/*
Resource epoch is used to signal the progression of the resource loading
process. The value is incremented by 1 each time a resource is updated
in the loading process.

A special *input* value of `null_epoch` is used to indicate that the
calling side has no resource yet, whether partial of full.

A special *output* value of `final_epoch` is used to indicate that the
loading process has finished and no more updates will come for the resource.

The calling side will likely replicate the following control flow:

    ResourceEpoch epoch = null_epoch;
    auto resource = co_await get_resource<SomeResource>(uuid, &epoch);
    initialize_from(resource);
    while (epoch != final_epoch) {
        auto resource = co_await get_resource<SomeResource>(uuid, &epoch);
        update_from(resource);
    }

Note that the `final_epoch` can be returned on the first request, either
in the case where the resource is already loaded or if the loading process
happened in a single step.
*/
using ResourceEpoch = uint32_t;


/*
A special *input* value of ResourceEpoch indicating that no resource
is yet held by the calling side. The calling side would likely initialize
its inout `epoch` variable with this value.
*/
constexpr ResourceEpoch null_epoch = 0;


/*
A special *output* value of the ResourceEpoch indicating that the resource
has been fully loaded.

NOTE: The integral value of `final_epoch` is not arbitrary, it must always hold
that `final_epoch > epoch` for any valid value of `epoch`, including `null_epoch`.
*/
constexpr ResourceEpoch final_epoch = -1;


/*
Indicates progress of a particular loading job with respect to completeness
of the requested resource.

This information only travels one way: from loading jobs to the loading context.
This is similar to the ResourceEpoch in spirit, but the control over the exact
value of the epoch is not given to loaders. Every update increments the epoch automatically.
*/
enum class ResourceProgress : bool {
    Incomplete, // Resource has only been loaded partially. More will come.
    Complete,   // Resource has been loaded to its full (all LODs, MIPs, etc.).
};


/*
The idea for now:

ResourceRegistry: map: RT -> Storage, API for insertion, query, removal, etc.

ResourceLoader: loader dispatch table: RT -> loader_func, support resources (thread pools, contexts, etc).
ResourceLoaderContext: Interface around ResourceLoader passed to loader functions.

ResourceUnpacker: unpacker dispatch table: (RT, dst_type) -> unpacker_func, support resources.
ResourceUnpackerContext: Interface arount ResourceUnpacker passed to unpacker functions.
*/
namespace detail {


template<ResourceType TypeV>
struct Storage;


template<ResourceType TypeV>
struct Entry {
    using resource_type = resource_traits<TypeV>::resource_type;
    using refcount_type = std::atomic<size_t>;

    std::unique_ptr<refcount_type> refcount;  // Refcount needs stable address. The flat hash table doesn't give you that.
    uint32_t                       mutex_idx; // NOTE: using 32-bit index to pack the structure better.
    ResourceEpoch                  epoch;
    resource_type                  resource;
};


template<ResourceType TypeV>
struct Storage {
    using resource_type    = resource_traits<TypeV>::resource_type;
    using entry_type       = Entry<TypeV>;
    using entry_mutex_type = std::shared_mutex;
    using map_type         = HashMap<UUID, entry_type>;
    using map_mutex_type   = std::shared_mutex;
    using mutex_type       = std::shared_mutex;
    using kv_type          = map_type::value_type; // I'm running out of words.

    static constexpr size_t mutex_pool_size = 32;
    static_assert(mutex_pool_size < uint32_t(-1));

    MutexPool<entry_mutex_type> entry_mutex_pool{ mutex_pool_size }; // For operations that modify each entry in the map.
    mutable map_mutex_type      map_mutex;                           // For operations that modify the map itself (insert/remove).
    map_type                    map;


    using pending_list_type = SmallVector<std::coroutine_handle<>, 2>;

    /*
    Can either be pending for each update, or only for the final epoch.
    We split into two lists since we do not want to needlessly rescan
    a list of N `only_final` entries on each incremental update, only
    to find out that none of them are interested in our update.

    We use a single map for all pending types and not map-per-type because
    we use the presence of *an entry* in a pending map as a signifier that
    the resource is currently being loaded.
    */
    struct PendingLists {
        pending_list_type incremental;
        pending_list_type only_final;
    };

    using pending_mutex_type = std::mutex; // TODO: Can be shared_mutex? Is there places where we only read?

    mutable pending_mutex_type  pending_mutex;
    HashMap<UUID, PendingLists> pending;

    // Map must be locked under "write" lock.
    //
    // Returns a pointer to the key-value of the new entry,
    // or a nullptr if the entry already exists.
    [[nodiscard]]
    auto new_entry(
        const UUID&                             uuid,
        resource_type                           resource,
        ResourceEpoch                           epoch,
        const std::unique_lock<map_mutex_type>& map_lock [[maybe_unused]])
            -> kv_type*
    {
        assert(map_lock.owns_lock());
        assert(map_lock.mutex() == &map_mutex);
        auto [it, was_emplaced] = map.try_emplace(uuid, entry_type{
            .refcount  = std::make_unique<typename entry_type::refcount_type>(0),
            .mutex_idx = uint32_t(entry_mutex_pool.new_mutex_idx()),
            .epoch     = epoch,
            .resource  = MOVE(resource),
        });
        return was_emplaced ? &(*it) : nullptr;
    }

    auto mutex_of(const entry_type& entry)
        -> entry_mutex_type&
    {
        return entry_mutex_pool[entry.mutex_idx];
    }

    [[nodiscard]]
    auto is_active_lock_of(const entry_type& entry, const auto& lock)
        -> bool
    {
        return lock.owns_lock() and lock.mutex() == &mutex_of(entry);
    }

    // Entry must be locked under "read" entry lock or stronger.
    template<template<typename...> typename LockT>
    [[nodiscard]]
    auto obtain_public(
        const kv_type&                 kv_pair,
        const LockT<entry_mutex_type>& entry_lock [[maybe_unused]])
            -> PublicResource<TypeV>
    {
        const UUID&       uuid  = kv_pair.first;
        const entry_type& entry = kv_pair.second;
        assert(is_active_lock_of(entry, entry_lock));

        const ResourceItem item = { .type=TypeV, .uuid=uuid };
        return {
            .resource = entry.resource,
            .usage    = { item, *entry.refcount }
        };
    }

    // Entry must be locked under "read" entry lock or stronger.
    template<template<typename...> typename LockT>
    [[nodiscard]]
    auto obtain_usage(
        const kv_type&                 kv_pair,
        const LockT<entry_mutex_type>& entry_lock [[maybe_unused]])
            -> ResourceUsage
    {
        const UUID&       uuid  = kv_pair.first;
        const entry_type& entry = kv_pair.second;
        assert(is_active_lock_of(entry, entry_lock));

        const ResourceItem item = { .type=TypeV, .uuid=uuid };
        return { item, *entry.refcount };
    }

    auto access_resource(
        entry_type&                               entry,
        const std::unique_lock<entry_mutex_type>& entry_lock [[maybe_unused]]) noexcept
            -> resource_type&
    {
        assert(is_active_lock_of(entry, entry_lock));
        return entry.resource;
    }

};

} // namespace detail


class ResourceLoaderContext;


/*
FIXME: Currently the registry is both a storage
and a loader of resources, which just makes a mess.
*/
class ResourceRegistry {
public:
    template<ResourceType TypeV>
    using storage_type = detail::Storage<TypeV>;
    template<ResourceType TypeV>
    using entry_type = detail::Entry<TypeV>;
    using any_map = Any<sizeof(HashMap<UUID, void*>)>; // Any value type will do.

    ResourceRegistry(
        ResourceDatabase&  resource_database,
        MeshRegistry&      mesh_registry,
        AsyncCradleRef     async_cradle
    )
        : resource_database_(resource_database)
        , mesh_registry_    (mesh_registry)
        , cradle_           (async_cradle)
    {}

    template<ResourceType TypeV>
    using loader_sig = Job<void>(ResourceLoaderContext, UUID); // FIXME: arg_type.

    template<ResourceType TypeV, of_signature<loader_sig<TypeV>> F>
    void register_resource(F&& loader);

    // If the `inout_epoch` parameter is nullptr, then the caller will
    // not be resumed on each incremental update, only on the final epoch.
    // This way, client unpackers that cannot handle incremental updates
    // can skip to full resource completeness.
    template<ResourceType TypeV>
    [[nodiscard]]
    auto get_resource(
        const UUID&    uuid,
        ResourceEpoch* inout_epoch = nullptr)
            -> awaiter<PublicResource<TypeV>> auto;


private:
    friend ResourceLoaderContext;

    using registry_type = HashMap<ResourceType, UniqueAny>;

    registry_type registry_;

    template<ResourceType TypeV>
    auto get_storage()
        -> storage_type<TypeV>&
    {
        auto it = registry_.find(TypeV);
        assert(it != registry_.end());
        return boost::any_cast<storage_type<TypeV>&>(it->second);
    }

    template<ResourceType TypeV>
    using loader_type     = UniqueFunction<loader_sig<TypeV>>; // FIXME: SBO function of some kind.
    using any_loader_type = UniqueAny; // TODO: Need MoveOnlyAny.
    using loaders_type    = HashMap<ResourceType, any_loader_type>;

    loaders_type loaders_;

    template<ResourceType TypeV>
    auto get_loader()
        -> loader_type<TypeV>&
    {
        auto it = loaders_.find(TypeV);
        assert(it != loaders_.end());
        return boost::any_cast<loader_type<TypeV>&>(it->second);
    }

    template<ResourceType TypeV>
    void start_loading(const UUID& uuid);


    ResourceDatabase&  resource_database_;
    MeshRegistry&      mesh_registry_;
    AsyncCradleRef     cradle_;
};


class ResourceLoaderContext {
public:
    // Basic helpers.
    auto& resource_database()  noexcept { return self_.resource_database_;         }
    auto& thread_pool()        noexcept { return self_.cradle_.loading_pool;       }
    auto& offscreen_context()  noexcept { return self_.cradle_.offscreen_context;  }
    auto& completion_context() noexcept { return self_.cradle_.completion_context; }
    auto& local_context()      noexcept { return self_.cradle_.local_context;      }

    // FIXME: This should be part of generic context in the loader.
    auto& mesh_registry()      noexcept { return self_.mesh_registry_; }

    // Resource progress.

    // Maybe take return value of the advance_fun as the new completeness state
    // instead of having another function.

    // We need some way to communicate the "progress" to the unpacker.
    // Maybe store it in the resource_type?

    // Synchronizing shared resources is a mess. How to sync MIP levels of a texture?
    // HINT: Try using texture views.

    // Create a new resource in the registry associated with the specified uuid
    // and resume the awaiters expecting the current epoch.
    //
    // All loaders must create or fail the resource they've been tasked with.
    //
    // If `progress` is Incomplete, then `update_resource()` must be called until
    // its update funciton returns Complete.
    template<ResourceType TypeV>
    [[nodiscard]]
    auto create_resource(
        const UUID&                           uuid,
        ResourceProgress                      progress,
        resource_traits<TypeV>::resource_type resource)
            -> ResourceUsage;

    // Update the data of the resource inplace and resume the awaiters of the new epoch.
    //
    // The update function should return Incomplete if more updates are expected and
    // Complete if this is the last update after which the resource is considered finalized.
    //
    // If Complete is returned, this function must not be called again by the same loading job.
    template<ResourceType TypeV, typename UpdateF>
        requires of_signature<UpdateF, ResourceProgress(typename resource_traits<TypeV>::resource_type&)>
    void update_resource(
        const UUID& uuid,
        UpdateF&&   update_fun);

    // Exception handling must be in-progress, `std::current_exception()` must be not null.
    // Right now, this can only be called *before* `create_resource()`.
    //
    // FIXME: With how the control has to flow with the try-block in the loaders,
    // the above is practically impossible to guarantee in a sane way.
    //
    // TODO: Think about cancellation for partial loads.
    template<ResourceType TypeV>
    void fail_resource(
        const UUID&        uuid,
        std::exception_ptr exception = std::current_exception());

    // Similar to calling get_resource<TypeV>() on the resource registry,
    // but returns a *private* resource for internal retention instead.
    template<ResourceType TypeV>
    [[nodiscard]]
    auto get_resource_dependency(
        const UUID&    uuid,
        ResourceEpoch* inout_epoch = nullptr)
            -> awaiter<PrivateResource<TypeV>> auto;

private:
    friend ResourceRegistry;
    ResourceLoaderContext(ResourceRegistry& self) : self_(self), task_guard_(self_.cradle_.task_counter) {}
    ResourceRegistry& self_;
    SingleTaskGuard   task_guard_;

    template<ResourceType TypeV>
    using storage_type = ResourceRegistry::storage_type<TypeV>;

    template<ResourceType TypeV>
    static void resolve_pending(
        storage_type<TypeV>& storage,
        const UUID&          uuid,
        ResourceEpoch        epoch,
        std::exception_ptr   exception);

};


template<ResourceType TypeV>
void ResourceLoaderContext::resolve_pending(
    storage_type<TypeV>& storage,
    const UUID&          uuid,
    ResourceEpoch        epoch,
    std::exception_ptr   exception) // NOLINT(performance-unnecessary-value-param)
{
    using pending_list_type = storage_type<TypeV>::pending_list_type;

    const bool final_resolve =
        (epoch == final_epoch) or exception;

    pending_list_type incremental;
    pending_list_type only_final;
    {
        const auto pending_lock = std::scoped_lock(storage.pending_mutex);
        const auto it = storage.pending.find(uuid);
        assert(it != storage.pending.end() && "Attempted to notify about resource update, but it is not pending.");

        auto& pending_lists = it->second;

        // We remove the entries from pending, they have to come back to pending
        // by calling get_resource() again later.
        incremental = MOVE(pending_lists.incremental);
        if (final_resolve) {
            only_final = MOVE(pending_lists.only_final);
        }

        // If this completes or fails the progress, then no one can be pending anymore.
        // Subsequent calls to get_resource() will instead return Complete cached entry,
        // or kick-off another load if this one failed.
        //
        // FIXME: This is part of the issue with infallabilty of partial loads.
        // Around here, we need to "remove" the entry from the main registry
        // if we failed the load midway through, but at the same time, the
        // unpacking side still retains the usage for partial item and
        // needs some way to decide whether to keep it or retry a load.
        if (final_resolve) {
            storage.pending.erase(it);
        }
    }

    auto resume_from_list = [&](pending_list_type& pending_list) {
        if (not exception) {
            // Manually resume each coroutine that was pending to "notify" it.
            // The caller is encouraged to reschedule somewhere else asap.
            for (std::coroutine_handle<>& handle : pending_list) {
                handle.resume();
            }
        } else {
            // This is particularly insane, but is required so that the
            // await_resume() of the woken up coroutine could see
            // std::current_exception() - it only returns non-null during
            // exception "handling" - effectively, inside a catch block.
            for (std::coroutine_handle<>& handle : pending_list) {
                try {
                    std::rethrow_exception(exception);
                } catch (...) {
                    // std::current_exception() can be called here.
                    handle.resume();
                }
            }
        }
    };

    resume_from_list(incremental);
    if (final_resolve) {
        resume_from_list(only_final);
    }
}


template<ResourceType TypeV>
auto ResourceLoaderContext::create_resource(
    const UUID&                           uuid,
    ResourceProgress                      progress,
    resource_traits<TypeV>::resource_type resource)
        -> ResourceUsage
{
    using storage_type = detail::Storage<TypeV>;
    using kv_type      = storage_type::kv_type;

    // NOTE: First epoch must be 1 and not 0, since 0 corresponds to
    // initial_epoch, which indicates a lack of a resource altogether.
    const ResourceEpoch epoch =
        (progress == ResourceProgress::Complete) ? final_epoch : 1;

    storage_type& storage = self_.get_storage<TypeV>();

    ResourceUsage usage = [&]{
        const auto map_lock = std::unique_lock(storage.map_mutex);
        // EWW: This allocates a new entry under a lock.
        // In particular, the heap allocation of refcount
        // would be nice to avoid, but that would mean
        // we'd have to do things more manually.
        kv_type* kv = storage.new_entry(uuid, MOVE(resource), epoch, map_lock);
        assert(kv && "Attempted to create a new resource, but it was already cached.");

        // Obtain "usage" before releasing the lock. This is important to guarantee
        // that the resource is still alive at least until we resolve all pending.
        const auto entry_lock = std::shared_lock(storage.mutex_of(kv->second));
        return storage.obtain_usage(*kv, entry_lock);
    }();

    // We hold the "usage" but no longer hold the locks.
    // Go grab the pending jobs and resume them one-by-one
    // so that they could obtain their own usage/resource copy.
    resolve_pending(storage, uuid, epoch, nullptr);

    // The loader needs to hold onto the usage to keep the resource alive.
    return usage;
}


template<ResourceType TypeV, typename UpdateF>
    requires of_signature<UpdateF, ResourceProgress(typename resource_traits<TypeV>::resource_type&)>
void ResourceLoaderContext::update_resource(
    const UUID& uuid,
    UpdateF&&   update_fun)
{
    using storage_type = detail::Storage<TypeV>;
    using entry_type   = storage_type::entry_type;

    storage_type& storage = self_.get_storage<TypeV>();

    const ResourceEpoch epoch = [&]{
        // Note that the locks are inverse of the create_resource().
        // Map is locked for read, since we don't create a new entry,
        // the entry is locked for write, since we update it.
        const auto map_lock = std::shared_lock(storage.map_mutex);
        entry_type* entry = try_find_value(storage.map, uuid);
        assert(entry && "Attempted to update a resource, but it did not exist.");

        const auto entry_lock = std::unique_lock(storage.mutex_of(*entry));
        assert(entry->epoch != final_epoch && "Attempted to update a resource, but it was already Complete.");

        // TODO: What if the update_fun throws? Should we go with the failure path maybe?
        const ResourceProgress new_progress =
            update_fun(storage.access_resource(*entry, entry_lock));

        if (new_progress == ResourceProgress::Complete) {
            entry->epoch = final_epoch;
        } else {
            ++entry->epoch;
        }

        return entry->epoch;
    }();

    resolve_pending(storage, uuid, epoch, nullptr);
}


template<ResourceType TypeV>
void ResourceLoaderContext::fail_resource(
    const UUID&        uuid,
    std::exception_ptr exception)
{
    using storage_type = detail::Storage<TypeV>;

    assert(exception && "Attempted to fail a load, but no exception is currently being handled. fail_resource() needs to be either called directly from inside a catch(...) block or the exception_ptr has to be obtained from another catch block and passed to fail_resource() manually.");

    storage_type& storage = self_.get_storage<TypeV>();

    const auto has_entry [[maybe_unused]] = [&]{
        const auto  map_lock = std::shared_lock(storage.map_mutex);
        const auto* kv       = try_find(storage.map, uuid);
        return bool(kv);
    };
    // FIXME: This is currently unnecessarily limiting, we need to be able to
    // properly cancel partially completed loads (or just notify the unpacking side).
    assert(not has_entry() && "Attempted to fail a load, but the entry was already created. FIXME.");

    resolve_pending(storage, uuid, final_epoch, exception);
}


template<ResourceType TypeV>
auto ResourceLoaderContext::get_resource_dependency(
    const UUID&    uuid,
    ResourceEpoch* inout_epoch)
        -> awaiter<PrivateResource<TypeV>> auto
{
    // TODO: This won't work if PublicResource is not convertible to
    // PrivateResource. Which it shouldn't be. But it is. For now.
    return self_.get_resource<TypeV>(uuid, inout_epoch);
}


template<ResourceType TypeV, of_signature<ResourceRegistry::loader_sig<TypeV>> F>
void ResourceRegistry::register_resource(F&& loader) {
    {
        auto [it, was_emplaced] = registry_.try_emplace(TypeV);
        assert(was_emplaced);
        it->second.emplace<storage_type<TypeV>>();
    }
    {
        auto [it, was_emplaced] = loaders_.try_emplace(TypeV, loader_type<TypeV>(FORWARD(loader)));
        assert(was_emplaced);
    }
}


template<ResourceType TypeV>
void ResourceRegistry::start_loading(const UUID& uuid) {
    loader_type<TypeV>& loader = get_loader<TypeV>();
    // Do we care to keep this job?
    // Or task counter is enough?
    loader(ResourceLoaderContext(*this), uuid);
}


template<ResourceType TypeV>
auto ResourceRegistry::get_resource(
    const UUID&    uuid,
    ResourceEpoch* inout_epoch)
        -> awaiter<PublicResource<TypeV>> auto
{
    using storage_type = detail::Storage<TypeV>;
    using entry_type   = detail::Entry<TypeV>;
    using kv_type      = storage_type::kv_type;

    assert(not inout_epoch or *inout_epoch != final_epoch &&
        "Input epoch cannot be final. No point requesting a resource when the final version is already held.");

    struct Awaiter {
        ResourceRegistry& self;
        storage_type&     storage;
        UUID              uuid;
        ResourceEpoch*    inout_epoch; // NOTE: Could be nullptr.

        // NOTE: Derived state, not meant to be specified on construction.
        const bool          only_final    = not inout_epoch;
        const ResourceEpoch current_epoch = only_final ? null_epoch : *inout_epoch;

        Optional<PublicResource<TypeV>> result = nullopt;

        auto await_ready()
            -> bool
        {
            const auto map_lock = std::shared_lock(storage.map_mutex);
            if (const kv_type* kv = try_find(storage.map, uuid)) {
                const entry_type& entry = kv->second;
                const auto entry_lock = std::shared_lock(storage.mutex_of(entry));
                if (_caller_wants(entry.epoch)) {
                    if (inout_epoch) *inout_epoch = entry.epoch;
                    result = storage.obtain_public(*kv, entry_lock);
                    return true;
                }
            }
            return false;
        }

        auto await_suspend(std::coroutine_handle<> h)
            -> bool
        {
            const auto map_lock = std::shared_lock(storage.map_mutex);
            if (const kv_type* kv = try_find(storage.map, uuid)) {
                const entry_type& entry = kv->second;
                const auto entry_lock = std::shared_lock(storage.mutex_of(entry));
                if (_caller_wants(entry.epoch)) {
                    if (inout_epoch) *inout_epoch = entry.epoch;
                    result = storage.obtain_public(*kv, entry_lock);
                    return false;
                }
            }

            const auto pending_lock = std::scoped_lock(storage.pending_mutex);
            auto [it, was_emplaced] = storage.pending.try_emplace(uuid);

            auto& pending_lists = it->second;
            if (only_final) { pending_lists.only_final .emplace_back(h); }
            else            { pending_lists.incremental.emplace_back(h); }

            if (was_emplaced) self.start_loading<TypeV>(uuid);

            return true;
        }

        [[nodiscard]]
        auto await_resume()
            -> PublicResource<TypeV>
        {
            if (auto exception = std::current_exception()) {
                // NOTE: This only works if this is called inside a catch block.
                // Currently, we try to ensure that inside resolve_pending().
                std::rethrow_exception(exception);
                // We could alternatively store the exception in the entry,
                // but we already store too much extra transient state there.
                // Maybe we could use another table for the transient stuff.
                // TODO: Think about potential locking issues related to that.
            }
            if (!result) {
                const auto map_lock = std::shared_lock(storage.map_mutex);
                const kv_type* kv = try_find(storage.map, uuid);
                assert(kv);
                const entry_type& entry = kv->second;
                const auto entry_lock = std::shared_lock(storage.mutex_of(entry));
                assert(_caller_wants(entry.epoch) && "Should never resume if the epoch is not wanted.");
                if (inout_epoch) *inout_epoch = entry.epoch;
                result = storage.obtain_public(*kv, entry_lock);
            }
            return move_out(result);
        }

        // If the calling side actually wants to be resumed to this epoch.
        auto _caller_wants(ResourceEpoch entry_epoch) const noexcept
            -> bool
        {
            return
                (not only_final and entry_epoch > current_epoch) or
                (only_final     and entry_epoch == final_epoch);
        }
    };

    return Awaiter{ *this, this->get_storage<TypeV>(), uuid, inout_epoch };
}


} // namespace josh
