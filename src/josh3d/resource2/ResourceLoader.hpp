#pragma once
#include "AsyncCradle.hpp"
#include "Resource.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceRegistry.hpp"
#include "MeshRegistry.hpp"


namespace josh {


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


class ResourceLoaderContext;


class ResourceLoader {
public:
    ResourceLoader(
        ResourceDatabase& resource_database,
        ResourceRegistry& resource_registry,
        MeshRegistry&     mesh_registry, // FIXME: Must be in a generic context instead.
        AsyncCradleRef    async_cradle
    )
        : resource_database_(resource_database)
        , resource_registry_(resource_registry)
        , mesh_registry_    (mesh_registry)
        , cradle_           (async_cradle)
    {}

    template<ResourceType TypeV,
        of_signature<Job<>(ResourceLoaderContext, UUID)> LoaderF>
    void register_loader(LoaderF&& loader);

    // This will either return a resource from cache directly, or suspend
    // until the resource is updated, and then resume with the new epoch.
    //
    // In order to track incremental updates, the caller must provide
    // the `inout_epoch` parameter. Before the first call to get_resource()
    // the caller would most likely want to initialize it to `null_epoch`.
    // The caller will be resumed for every epoch that is greater than
    // the epoch provided in the `inout_epoch` parameter.
    //
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

    // Submit a job to await completion of a resource with the specified uuid.
    // This is a simplified wrapper around get_resource() and is equivalent to:
    //
    //   `co_return co_await get_resource<TypeV>(uuid);`
    //
    // meaning no incremental loading is possible, and the job is launched
    // regardless of whether the asset is already cached or not.
    template<ResourceType TypeV>
    auto load(UUID uuid)
        -> Job<PublicResource<TypeV>>;

private:
    friend ResourceLoaderContext;
    ResourceDatabase& resource_database_;
    ResourceRegistry& resource_registry_;
    MeshRegistry&     mesh_registry_;
    AsyncCradleRef    cradle_;

    using key_type    = ResourceType;
    using loader_func = UniqueFunction<Job<>(ResourceLoaderContext, UUID)>;
    using dtable_type = HashMap<key_type, loader_func>;

    dtable_type dispatch_table_{};

    auto _start_loading(const key_type& key, const UUID& uuid)
        -> Job<>;
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
    //
    // FIXME: This should not be needed as long as all dependencies are
    // opaque UUIDs.
    template<ResourceType TypeV>
    [[nodiscard]]
    auto get_resource_dependency(
        const UUID&    uuid,
        ResourceEpoch* inout_epoch = nullptr)
            -> awaiter<PrivateResource<TypeV>> auto;

private:
    friend ResourceLoader;
    ResourceLoaderContext(ResourceLoader& self) : self_(self), task_guard_(self_.cradle_.task_counter) {}
    ResourceLoader& self_;
    SingleTaskGuard task_guard_;

    template<ResourceType TypeV>
    static void resolve_pending(
        ResourceRegistry::Storage<TypeV>& storage,
        const UUID&                       uuid,
        ResourceEpoch                     epoch,
        std::exception_ptr                exception);

};




template<ResourceType TypeV,
    of_signature<Job<>(ResourceLoaderContext, UUID)> LoaderF>
void ResourceLoader::register_loader(LoaderF&& loader) {
    const key_type key = TypeV;
    auto [it, was_emplaced] = dispatch_table_.try_emplace(key, FORWARD(loader));
    assert(was_emplaced);
    resource_registry_.initialize_storage_for<TypeV>();
}


template<ResourceType TypeV>
auto ResourceLoader::get_resource(
    const UUID&    uuid,
    ResourceEpoch* inout_epoch)
        -> awaiter<PublicResource<TypeV>> auto
{
    using storage_type = ResourceRegistry::Storage<TypeV>;
    using entry_type   = ResourceRegistry::Entry<TypeV>;
    using kv_type      = storage_type::kv_type;

    assert(not inout_epoch or *inout_epoch != final_epoch &&
        "Input epoch cannot be final. No point requesting a resource when the final version is already held.");

    struct Awaiter {
        ResourceLoader& self;
        storage_type&   storage;
        UUID            uuid;
        ResourceEpoch*  inout_epoch; // NOTE: Could be nullptr.

        // NOTE: Derived helpers, not meant to be specified on construction.
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

            if (was_emplaced) self._start_loading(TypeV, uuid);

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

    return Awaiter{ *this, resource_registry_.get_storage<TypeV>(), uuid, inout_epoch };
}


template<ResourceType TypeV>
auto ResourceLoader::load(UUID uuid)
    -> Job<PublicResource<TypeV>>
{
    co_return co_await get_resource<TypeV>(uuid);
}


template<ResourceType TypeV>
void ResourceLoaderContext::resolve_pending(
    ResourceRegistry::Storage<TypeV>& storage,
    const UUID&                       uuid,
    ResourceEpoch                     epoch,
    std::exception_ptr                exception)
{
    using pending_list_type = ResourceRegistry::Storage<TypeV>::pending_list_type;

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
    using storage_type = ResourceRegistry::Storage<TypeV>;
    using kv_type      = storage_type::kv_type;

    // NOTE: First epoch must be 1 and not 0, since 0 corresponds to
    // null_epoch, which indicates a lack of a resource altogether.
    const ResourceEpoch epoch =
        (progress == ResourceProgress::Complete) ? final_epoch : null_epoch + 1;

    storage_type& storage = self_.resource_registry_.get_storage<TypeV>();

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
    using storage_type = ResourceRegistry::Storage<TypeV>;
    using entry_type   = storage_type::entry_type;

    storage_type& storage = self_.resource_registry_.get_storage<TypeV>();

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
    using storage_type = ResourceRegistry::Storage<TypeV>;

    assert(exception && "Attempted to fail a load, but no exception is currently being handled. fail_resource() needs to be either called directly from inside a catch(...) block or the exception_ptr has to be obtained from another catch block and passed to fail_resource() manually.");

    storage_type& storage = self_.resource_registry_.get_storage<TypeV>();

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


} // namespace josh
