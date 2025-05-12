#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "Resource.hpp"
#include "ContainerUtils.hpp"
#include "MutexPool.hpp"
#include "ResourceInfo.hpp"
#include "RuntimeError.hpp"
#include "UUID.hpp"
#include <fmt/core.h>
#include <cassert>
#include <cstdint>
#include <coroutine>
#include <atomic>
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
process. The value is incremented each time a resource is updated
in the loading process.

A special *input* value of `null_epoch` is used to indicate that the
calling side has no resource yet, whether partial of full.

A special *output* value of `final_epoch` is used to indicate that the
loading process has finished and no more updates will come for the resource.

If incremental loading is desired, the calling side will likely have to
replicate the following control flow:

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
A collection of resource-associated storage types that map each resource UUID
to a resource in its intermediate retained state.

NOTE: This is mostly a low-level implementation component, most of the storage
and entry fields are public, and locks have to be taken manually. This is done
in order to not hamper "creative uses" of the registry by other systems.
Specialized functions or dedicated "thing do'er" classes should likely be used
to interact with the registry entries in a correct and meaningful way.
*/
class ResourceRegistry {
public:
    template<ResourceType TypeV>
    struct Storage;
    template<ResourceType TypeV>
    struct Entry;

    // Creates a storage for the specified resource type in the registry.
    // Returns true if the storage was initialized, false if it already exists.
    template<ResourceType TypeV>
    auto initialize_storage_for()
        -> bool
    {
        auto [it, was_emplaced] = registry_.try_emplace(TypeV);
        if (was_emplaced) it->second.emplace<Storage<TypeV>>();
        return was_emplaced;
    }

    // Get a reference to the storage of the specified resource type.
    // Will throw if the storage for this type is not in the registry.
    template<ResourceType TypeV>
    auto get_storage()
        -> Storage<TypeV>&
    {
        if (auto* storage = try_get_storage<TypeV>()) {
            return *storage;
        } else {
            throw RuntimeError(fmt::format("No storage found for resource type: {}.", resource_info().name_or_id(TypeV)));
        }
    }

    // Get a pointer to the storage of the specified resource type.
    // Will return nullptr the storage for this type is not in the registry.
    template<ResourceType TypeV>
    auto try_get_storage()
        -> Storage<TypeV>*
    {
        if (any_storage_type* any_storage = try_find_value(registry_, TypeV)) {
            auto* ptr = any_cast<Storage<TypeV>>(any_storage);
            assert(ptr && "Storage entry exists, but the type is mismatched.");
            return ptr;
        } else {
            return nullptr;
        }
    }

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

    private:
        static constexpr size_t mutex_pool_size = 32; // Has to fit in uint32_t.
        static_assert(mutex_pool_size < uint32_t(-1));
        MutexPool<entry_mutex_type> entry_mutex_pool_{ mutex_pool_size }; // For operations that modify each entry in the map.
    public:
        mutable map_mutex_type      map_mutex;                            // For operations that modify the map itself (insert/remove).
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

        // Returns a pointer to the key-value of the new entry,
        // or a nullptr if the entry already exists.
        //
        // Map must be locked under "write" lock.
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
                .mutex_idx = uint32_t(entry_mutex_pool_.new_mutex_idx()),
                .epoch     = epoch,
                .resource  = MOVE(resource),
            });
            return was_emplaced ? &(*it) : nullptr;
        }

        auto mutex_of(const entry_type& entry)
            -> entry_mutex_type&
        {
            return entry_mutex_pool_[entry.mutex_idx];
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
            assert(_is_active_lock_of(entry, entry_lock));

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
            assert(_is_active_lock_of(entry, entry_lock));

            const ResourceItem item = { .type=TypeV, .uuid=uuid };
            return { item, *entry.refcount };
        }

        auto access_resource(
            entry_type&                               entry,
            const std::unique_lock<entry_mutex_type>& entry_lock [[maybe_unused]]) noexcept
                -> resource_type&
        {
            assert(_is_active_lock_of(entry, entry_lock));
            return entry.resource;
        }

        [[nodiscard]]
        auto _is_active_lock_of(const entry_type& entry, const auto& lock)
            -> bool
        {
            return lock.owns_lock() and lock.mutex() == &mutex_of(entry);
        }

    };

private:
    using key_type         = ResourceType;
    using any_storage_type = UniqueAny;
    using registry_type    = HashMap<key_type, any_storage_type>;

    registry_type registry_;
};


} // namespace josh
