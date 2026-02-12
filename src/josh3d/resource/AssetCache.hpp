#pragma once
#include "Asset.hpp"
#include "Coroutines.hpp"
#include <exception>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <shared_mutex>


namespace josh {


/*
Async asset cache with support for coroutines.
*/
class AssetCache {
public:
    template<AssetKind KindV> struct GetIfCachedAwaiter;

    // Returns the result without suspending if it is cached, or:
    // - If the request is currently pending completion, suspends until it is resolved by a loading job.
    // - If the request is new, returns a nullopt; this coroutine *must* load the asset
    //   and call `cache(fail)_and_resolve_pending()` before `co_return`ing (or terminating with an exception).
    template<AssetKind KindV>
    auto get_if_cached_or_join_pending(const AssetPath& path)
        -> GetIfCachedAwaiter<KindV>;

    // Cache and resolve the pending requests on `path` with a value.
    //
    // This must be done from the coroutine that resumed from
    // `get_if_cached_or_join_pending()` without the cached asset (nullopt),
    // and successfully obtained the result.
    //
    // This does *not* return an awaitable, since there's nothing to suspend for.
    //
    // Returns the number of pending requests resolved.
    template<AssetKind KindV>
    auto cache_and_resolve_pending(const AssetPath& path, const StoredAsset<KindV>& result)
        -> size_t;

    // Resolve the pending requests on `path` with an exception.
    //
    // This must be done from the coroutine that resumed from
    // `get_if_cached_or_join_pending()` without the cached asset (nullopt),
    // and failed to obtain the result / terminated with an exception.
    //
    // This does *not* return an awaitable, since there's nothing to suspend for.
    //
    // Returns the number of pending requests resolved.
    template<AssetKind KindV>
    auto fail_and_resolve_pending(const AssetPath& path, std::exception_ptr exception = std::current_exception())
        -> size_t;

private:
    template<AssetKind KindV>
    struct Storage {
        static constexpr AssetKind asset_kind = KindV;
        using key_type      = AssetPath;
        using shared_handle = Job<SharedAsset<KindV>>::shared_handle_type;
        using CacheMap      = std::unordered_map<key_type, StoredAsset<KindV>>;
        using PendingList   = std::vector<shared_handle>;
        using PendingMap    = std::unordered_map<key_type, PendingList>;

        CacheMap                  cache;
        PendingMap                pending;
        mutable std::shared_mutex cache_mutex;
        mutable std::mutex        pending_mutex;
    };

    template<AssetKind ...KindVs> using StorageArrayT = std::tuple<Storage<KindVs>...>;
    using StorageArray = StorageArrayT<AssetKind::Model, AssetKind::Texture, AssetKind::Cubemap>;

    StorageArray caches_;

    template<AssetKind KindV> auto storage_for()       noexcept ->       Storage<KindV>& { return get<Storage<KindV>>(caches_); }
    template<AssetKind KindV> auto storage_for() const noexcept -> const Storage<KindV>& { return get<Storage<KindV>>(caches_); }

};




template<AssetKind KindV>
struct AssetCache::GetIfCachedAwaiter {
    using storage_type = Storage<KindV>;
    using key_type     = storage_type::key_type;
    storage_type&   storage;
    const key_type& key;

    std::optional<SharedAsset<KindV>>    cached_result = std::nullopt;
    Job<SharedAsset<KindV>>::handle_type handle        = nullptr;

    bool await_ready() {
        {
            // It is quite unlikely that we actually encounter a pending request,
            // so adjust our access accordingly.
            const std::shared_lock cache_lock{ storage.cache_mutex };
            if (const auto* item = try_find(storage.cache, key)) {
                cached_result = item->second;
                return true;
            }
            return false;
        }
    }

    bool await_suspend(Job<SharedAsset<KindV>>::handle_type h) {
        {
            handle = h; // Stash the handle for later.
            const std::shared_lock cache_lock{ storage.cache_mutex };
            if (const auto* item = try_find(storage.cache, key)) {
                cached_result = item->second;
                return false;
            }
            {
                const std::scoped_lock pending_lock{ storage.pending_mutex };
                auto [it, was_emplaced] = storage.pending.try_emplace(key);

                // If we just emplaced a new entry, then don't actually add ourselves to pending,
                // we'll be the ones resolving this request. Don't suspend, resume with a nullopt.
                if (was_emplaced) {
                    return false;
                }

                // Lastly, if there is already a pending list for this asset, then we just add
                // ourselves to it and suspend. Our result will be resolved by the first job
                // that emplaced an entry into the pending list.
                //
                // NOTE: Ownership is taken away from the promise and transferred to the pending list.

                // FIXME: This has a nasty bug later, where if the user side discards its ownership
                // then upon erasing the list when resumed from pending, the coroutine will just be destroyed.
                // I have no idea what I was thinking. We should not release ownership here at all.
                it->second.emplace_back(h.promise().release_ownership());
                return true;
            }
        }
    }

    auto await_resume() -> std::optional<SharedAsset<KindV>> {
        using promise_type = Job<SharedAsset<KindV>>::promise_type;
        if (cached_result) {
            // Either we found the result in the cache and can resume early.
            return MOVE(cached_result);
        }
        promise_type& p = handle.promise();
        if (!is<typename promise_type::no_result>(p.get_result_value())) {
            // Or the result became available through another job directly emplacing it.
            //
            // NOTE: Directly setting the result through `set_result_value()` does not
            // set the `is_ready()` flag, so we check the currently contained value instead.
            //
            // Also NOTE: We *must* extract the result out, else the precondition of the
            // `return_value()`: `is<no_result>(result_value())`, is violated.
            //
            // We use the normal `extract_result()` instead of manual `extract_result_value()`
            // because the former will rethrow the exception which will make the resumed coroutine
            // follow the normal flow of suspension and termination after that.
            return p.extract_result();
        }
        // Otherwise, we never found the result, and must resume to load it ourselves.
        return std::nullopt;
    }

};




template<AssetKind KindV>
auto AssetCache::get_if_cached_or_join_pending(const AssetPath& path)
    -> GetIfCachedAwaiter<KindV>
{
    return { storage_for<KindV>(), path };
}




template<AssetKind KindV>
auto AssetCache::cache_and_resolve_pending(const AssetPath& path, const StoredAsset<KindV>& result)
    -> size_t
{
    Storage<KindV>& storage = storage_for<KindV>();

    typename Storage<KindV>::PendingList pending_list;

    {
        const std::unique_lock cache_lock{ storage.cache_mutex };
        auto [it, was_emplaced] = storage.cache.emplace(path, result);
        assert(was_emplaced && "Attempted to resolve a request by caching an asset, but it was already cached.");
        {
            const std::scoped_lock pending_lock{ storage.pending_mutex };
            const auto it = storage.pending.find(path);

            assert(it != storage.pending.end() && "Attempted to resolve a pending request, but it was not marked as such before.");

            pending_list = MOVE(it->second); // Just move the pending list out to not keep the lock for too long.

            storage.pending.erase(it);
        }
    }

    // We can resolve pending outside of the locks, since we give no guarantees on the order of
    // resolution for pending request with respect to when caching happens.
    for (auto& handle : pending_list) {
        // Manually emplace the result into each coroutine and resume it so that it can `co_return` it.
        handle->promise().set_result_value(result);
        handle->resume();
    }

    return pending_list.size();
}




template<AssetKind KindV>
auto AssetCache::fail_and_resolve_pending(const AssetPath& path, std::exception_ptr exception)
    -> size_t
{
    Storage<KindV>& storage = storage_for<KindV>();

    typename Storage<KindV>::PendingList pending_list;

    {
        // Only lock the pending, since we are not caching this result.
        const std::scoped_lock pending_lock{ storage.pending_mutex };
        const auto it = storage.pending.find(path);

        assert(it != storage.pending.end() && "Attempted to resolve a pending request, but it was not marked as such before.");

        pending_list = MOVE(it->second);

        storage.pending.erase(it);
    }

    for (auto& handle : pending_list) {
        // Manually emplace the exception into each coroutine and resume it so that it can terminate with it.
        handle->promise().set_result_value(exception);
        handle->resume();
    }

    return pending_list.size();
}




} // namespace josh
