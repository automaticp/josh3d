#pragma once
#include "AnyRef.hpp"
#include "AsyncCradle.hpp"
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "CommonConcepts.hpp"
#include "CompletionContext.hpp"
#include "LocalContext.hpp"
#include "MeshRegistry.hpp"
#include "OffscreenContext.hpp"
#include "Resource.hpp"
#include "ContainerUtils.hpp"
#include "Coroutines.hpp"
#include "HashedString.hpp"
#include "MutexPool.hpp"
#include "ResourceDatabase.hpp"
#include "TaskCounterGuard.hpp"
#include "ThreadPool.hpp"
#include "UUID.hpp"
#include "UniqueFunction.hpp"
#include <boost/any/unique_any.hpp>
#include <boost/any/basic_any.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <entt/core/fwd.hpp>
#include <entt/entity/fwd.hpp>
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


enum class ResourceProgress : bool {
    Incomplete, // Resource has only been loaded partially. Come back for more.
    Complete,   // Resource has been loaded to its full (all LODs, MIPs, etc.).
};


template<ResourceType TypeV> struct resource_traits;


/*
Used for SBO size probing of internal datastructures.
And as a primitive example. Cannot be used otherwise.
*/
struct DummyResource {};
constexpr auto DummyResource = "DummyResource"_hs;


template<> struct resource_traits<DummyResource> {
    using resource_type = struct DummyResource;
};


template<ResourceType TypeV>
struct PublicResource {
    // This should be a "const" resource reference.
    using resource_type = resource_traits<TypeV>::resource_type;
    resource_type resource;
    ResourceUsage usage;
};


// TODO: This should have different semantics.
template<ResourceType TypeV>
using PrivateResource = PublicResource<TypeV>;


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
class Entry {
public:
    using resource_type = resource_traits<TypeV>::resource_type;
    using refcount_type = std::atomic<size_t>;
    using mutex_type    = std::shared_mutex;

    auto mutex_ptr() const noexcept -> mutex_type*      { return mutex_ptr_;  }
    auto mutex()     const noexcept -> mutex_type&      { return *mutex_ptr_; }
    auto progress()  const noexcept -> ResourceProgress { return progress_;   }

private:
    // Needs stable address. The hash table doesn't give you that.
    std::unique_ptr<refcount_type> refcount_ = std::make_unique<refcount_type>(0);
    mutex_type*                    mutex_ptr_;
    resource_type                  resource_;
    ResourceProgress               progress_ = ResourceProgress::Incomplete;

    // NOTE: The above can be packed better. Progress can be packed
    // into the refcount bits, mutex can be a u8 index, etc.

    friend Storage<TypeV>;

    // Must be called by Storage to guarantee that mutex is from the right pool.
    Entry(resource_type resource, ResourceProgress progress, mutex_type& entry_mutex)
        : progress_ { progress       }
        , mutex_ptr_{ &entry_mutex   }
        , resource_ { MOVE(resource) }
    {}

    // Must be called by Storage since we don't know our own UUID.
    // TODO: Isn't this kinda dumb?
    auto obtain_public(const UUID& uuid) const noexcept
        -> PublicResource<TypeV>
    {
        return {
            .resource = resource_, // Copy
            .usage    = obtain_usage(uuid),
        };
    }

    // Must be called by Storage since we don't know our own UUID.
    auto obtain_usage(const UUID& uuid) const noexcept
        -> ResourceUsage
    {
        const ResourceItem item{ .type = TypeV, .uuid = uuid };
        return { item, *refcount_ };
    }

};


template<ResourceType TypeV>
struct Storage {
    using resource_type  = resource_traits<TypeV>::resource_type;
    using entry_type     = Entry<TypeV>;
    using map_type       = HashMap<UUID, entry_type>;
    using kv_type        = map_type::value_type; // I'm running out of words.

    mutable std::shared_mutex    map_mutex;              // For operations that modify the map itself (insert/remove).
    MutexPool<std::shared_mutex> entry_mutex_pool{ 32 }; // For operations that modify each entry in the map.
    map_type                     map;

    // Map must be locked under "write" lock.
    //
    // Returns a pointer to the key-value of the new entry,
    // or a nullptr if the entry already exists.
    [[nodiscard]]
    auto new_entry(
        const UUID&                                uuid,
        resource_type                              resource,
        ResourceProgress                           progress,
        const std::unique_lock<std::shared_mutex>& active_map_lock [[maybe_unused]])
            -> kv_type*
    {
        assert(active_map_lock.owns_lock());
        assert(active_map_lock.mutex() == &map_mutex);
        auto [it, was_emplaced] =
            map.try_emplace(uuid, entry_type(MOVE(resource), progress, *entry_mutex_pool.new_mutex()));
        return was_emplaced ? &(*it) : nullptr;
    }

    // Entry must be locked under "read" entry lock or stronger.
    template<template<typename...> typename LockT>
    [[nodiscard]]
    static auto obtain_public(
        const kv_type&                  kv_pair,
        const LockT<std::shared_mutex>& active_entry_lock [[maybe_unused]])
            -> PublicResource<TypeV>
    {
        assert(active_entry_lock.owns_lock());
        assert(active_entry_lock.mutex() == kv_pair.second.mutex_ptr());
        return kv_pair.second.obtain_public(kv_pair.first);
    }

    // Entry must be locked under "read" entry lock or stronger.
    template<template<typename...> typename LockT>
    [[nodiscard]]
    static auto obtain_usage(
        const kv_type&                  kv_pair,
        const LockT<std::shared_mutex>& active_entry_lock [[maybe_unused]])
            -> ResourceUsage
    {
        assert(active_entry_lock.owns_lock());
        assert(active_entry_lock.mutex() == kv_pair.second.mutex_ptr());
        return kv_pair.second.obtain_usage(kv_pair.first);
    }

    static auto access_resource(
        entry_type&                                entry,
        const std::unique_lock<std::shared_mutex>& active_entry_lock [[maybe_unused]]) noexcept
            -> resource_type&
    {
        assert(active_entry_lock.owns_lock());
        assert(active_entry_lock.mutex() == entry.mutex_ptr());
        return entry.resource_;
    }

    static void mark_complete(
        entry_type&                                entry,
        const std::unique_lock<std::shared_mutex>& active_entry_lock [[maybe_unused]]) noexcept
    {
        assert(active_entry_lock.owns_lock());
        assert(active_entry_lock.mutex() == entry.mutex_ptr());
        entry.progress_ = ResourceProgress::Complete;
    }

    using pending_list = SmallVector<std::coroutine_handle<>, 4>;

    mutable std::mutex          pending_mutex;
    HashMap<UUID, pending_list> pending; // TODO: Do we need this per-resource?
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

    template<ResourceType TypeV>
    [[nodiscard]]
    auto get_resource(const UUID& uuid, ResourceProgress* out_progress = nullptr)
        -> awaiter<PublicResource<TypeV>> auto;


private:
    friend ResourceLoaderContext;

    using registry_type =
        HashMap<ResourceType, UniqueAny>;

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

    // If `progress` is Incomplete, then `update_resource()` must be called until
    // its update funciton returns Complete.
    template<ResourceType TypeV>
    [[nodiscard]]
    auto create_resource(
        const UUID&                           uuid,
        ResourceProgress                      progress,
        resource_traits<TypeV>::resource_type resource)
            -> ResourceUsage;

    template<ResourceType TypeV, typename UpdateF>
        requires of_signature<UpdateF, ResourceProgress(typename resource_traits<TypeV>::resource_type&)>
    void update_resource(const UUID& uuid, UpdateF&& update_fun);

    // Exception handling must be in-progress, `std::current_exception()` must be not null.
    // Right now, this can only be called *before* `create_resource()`.
    //
    // FIXME: With how the control has to flow with the try-block in the loaders,
    // the above is practically impossible to guarantee in a sane way.
    //
    // TODO: Think about cancellation for partial loads.
    template<ResourceType TypeV>
    void fail_resource(const UUID& uuid, std::exception_ptr exception = std::current_exception());

    // Similar to calling get_resource<TypeV>() on the resource registry,
    // but returns a *private* resource for internal retention instead.
    template<ResourceType TypeV>
    [[nodiscard]]
    auto get_resource_dependency(
        const UUID&       uuid,
        ResourceProgress* out_progress = nullptr)
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
        bool                 final_resolve,
        std::exception_ptr   exception);

};


template<ResourceType TypeV>
void ResourceLoaderContext::resolve_pending(
    storage_type<TypeV>& storage,
    const UUID&          uuid,
    bool                 final_resolve,
    std::exception_ptr   exception)
{
    typename storage_type<TypeV>::pending_list pending_list;
    {
        const auto pending_lock = std::scoped_lock(storage.pending_mutex);
        const auto it = storage.pending.find(uuid);
        assert(it != storage.pending.end() && "Attempted to notify about a new resource, but it is not pending.");

        // We remove the entries from pending, they have to come back to pending
        // by calling get_resource() again later.
        pending_list = MOVE(it->second);

        // If this completes or fails the progress, then no one can be pending anymore.
        // Subsequent calls to get_resource() will instead return Complete cached entry,
        // or kick-off another load if this one failed.
        if (final_resolve || exception) {
            storage.pending.erase(it);
        }
    }

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
}


template<ResourceType TypeV>
[[nodiscard]]
auto ResourceLoaderContext::create_resource(
    const UUID&                           uuid,
    ResourceProgress                      progress,
    resource_traits<TypeV>::resource_type resource)
        -> ResourceUsage
{
    using storage_type = storage_type<TypeV>;
    using kv_type      = storage_type::kv_type;

    storage_type& storage = self_.get_storage<TypeV>();

    ResourceUsage usage = [&]{
        const auto map_lock = std::unique_lock(storage.map_mutex);
        // EWW: This allocates a new entry under a lock.
        // In particular, the heap allocation of refcount
        // would be nice to avoid, but that would mean
        // we'd have to do things more manually.
        kv_type* kv = storage.new_entry(uuid, MOVE(resource), progress, map_lock);
        assert(kv && "Attempted to create a new resource, but it was already cached.");

        // Obtain "usage" before releasing the lock. This is important to guarantee
        // that the resource is still alive at least until we resolve all pending.
        const auto entry_lock = std::shared_lock(kv->second.mutex());
        return storage_type::obtain_usage(*kv, entry_lock);
    }();

    // We hold the "usage" but no longer hold the locks.
    // Go grab the pending jobs and resume them one-by-one
    // so that they could obtain their own usage/resource copy.
    const bool final_resolve = (progress == ResourceProgress::Complete);
    resolve_pending(storage, uuid, final_resolve, nullptr);

    // The loader needs to hold onto the usage to keep the resource alive.
    return usage;
}


template<ResourceType TypeV, typename UpdateF>
    requires of_signature<UpdateF, ResourceProgress(typename resource_traits<TypeV>::resource_type&)>
void ResourceLoaderContext::update_resource(
    const UUID& uuid,
    UpdateF&&   update_fun)
{
    using storage_type = storage_type<TypeV>;
    using kv_type      = storage_type::kv_type;
    using entry_type   = storage_type::entry_type;

    storage_type& storage = self_.get_storage<TypeV>();

    const ResourceProgress progress = [&]{
        // Note that the locks are inverse of the create_resource().
        // Map is locked for read, since we don't create a new entry,
        // the entry is locked for write, since we update it.
        const auto map_lock = std::shared_lock(storage.map_mutex);
        entry_type* entry = try_find_value(storage.map, uuid);
        assert(entry && "Attempted to update a resource, but it did not exist.");

        const auto entry_lock = std::unique_lock(entry->mutex());
        assert(entry->progress() != ResourceProgress::Complete && "Attempted to update a resource, but it was already Complete.");

        // TODO: What if the update_fun throws? Should we go with the failure path maybe?
        const ResourceProgress new_progress =
            update_fun(storage_type::access_resource(*entry, entry_lock));

        if (new_progress == ResourceProgress::Complete) {
            storage_type::mark_complete(*entry, entry_lock);
        }

        return new_progress;
    }();

    const bool final_resolve = (progress == ResourceProgress::Complete);
    resolve_pending(storage, uuid, final_resolve, nullptr);
}


template<ResourceType TypeV>
void ResourceLoaderContext::fail_resource(
    const UUID&        uuid,
    std::exception_ptr exception)
{
    assert(exception && "Attempted to fail a load, but no exception is currently being handled. fail_resource() needs to be either called directly from inside a catch(...) block or the exception_ptr has to be obtained from another catch block and passed to fail_resource() manually.");

    using storage_type = storage_type<TypeV>;
    storage_type& storage = self_.get_storage<TypeV>();

    const auto has_entry [[maybe_unused]] = [&]{
        const auto  map_lock = std::shared_lock(storage.map_mutex);
        const auto* kv       = try_find(storage.map, uuid);
        return bool(kv);
    };
    assert(!has_entry() && "Attempted to fail a load, but the entry was already created.");

    const bool final_resolve = true;
    resolve_pending(storage, uuid, final_resolve, exception);
}


template<ResourceType TypeV>
[[nodiscard]]
auto ResourceLoaderContext::get_resource_dependency(
    const UUID&       uuid,
    ResourceProgress* out_progress)
        -> awaiter<PrivateResource<TypeV>> auto
{
    return self_.get_resource<TypeV>(uuid, out_progress);
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
[[nodiscard]]
auto ResourceRegistry::get_resource(const UUID& uuid, ResourceProgress* out_progress)
    -> awaiter<PublicResource<TypeV>> auto
{
    using storage_type = storage_type<TypeV>;
    using entry_type   = entry_type<TypeV>;
    using kv_type      = storage_type::kv_type;

    struct Awaiter {
        ResourceRegistry& self;
        storage_type&     storage;
        UUID              uuid;
        ResourceProgress* out_progress;

        std::optional<PublicResource<TypeV>> result = std::nullopt;

        bool await_ready() {
            const auto map_lock = std::shared_lock(storage.map_mutex);
            if (const kv_type* kv = try_find(storage.map, uuid)) {
                const entry_type& entry = kv->second;
                const auto entry_lock = std::shared_lock(entry.mutex());
                if (out_progress) *out_progress = entry.progress();
                result = storage_type::obtain_public(*kv, entry_lock);
                return true;
            }
            return false;
        }

        bool await_suspend(std::coroutine_handle<> h) {
            const auto map_lock = std::shared_lock(storage.map_mutex);
            if (const kv_type* kv = try_find(storage.map, uuid)) {
                const entry_type& entry = kv->second;
                const auto entry_lock = std::shared_lock(entry.mutex());
                if (out_progress) *out_progress = entry.progress();
                result = storage_type::obtain_public(*kv, entry_lock);
                return false;
            }

            const auto pending_lock = std::scoped_lock(storage.pending_mutex);
            auto [it, was_emplaced] = storage.pending.try_emplace(uuid);
            it->second.emplace_back(h);

            if (was_emplaced) {
                self.start_loading<TypeV>(uuid);
            }

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
                const auto entry_lock = std::shared_lock(entry.mutex());
                if (out_progress) *out_progress = entry.progress();
                result = storage_type::obtain_public(*kv, entry_lock);
            }
            return move_out(result);
        }
    };

    return Awaiter{ *this, this->get_storage<TypeV>(), uuid, out_progress };
}


} // namespace josh
