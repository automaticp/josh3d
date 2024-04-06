#pragma once
#include "ImageData.hpp"
#include "Filesystem.hpp"
#include "Shared.hpp"
#include "Future.hpp"
#include "TextureHelpers.hpp"
#include "ThreadsafeQueue.hpp"
#include "ThreadPool.hpp"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <exception>
#include <memory>
#include <optional>
#include <stop_token>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>


namespace josh {


/*

There are, technically, two AsyncPools, one is for Data, and the other
is for GL Objects. Extra diffuculty arises because they have to work
together to transfer data from the hard drive, to the vram.

For example, a simple load request done by the rendering system
would have to go through both of the Pools:

1. Rendering System calls AsyncGLObjectPool::load(path) from Main Thread
to load a Resource (Texture/Model/etc.) and recieves a future to the resource.

2. AsyncGLObjectPool checks the cache for an existing instance
and finds none. Dispatches some Thread A to load raw data for the Resource.

3. Thread A calls AsyncDataPool::load(path) to request a load of
the raw resource, and waits until it is complete.

4. AsyncDataPool checks the cache for an existing instance
and finds none. Dispatches some Thread B to load raw data for the Resource.

5. Thread B calls load_data_from_file(path) or similar, which performs
the IO and loads the raw data from the hard drive.

6. Thread B then caches the result into AsyncDataPool and returns
a shared handle of the raw data for the Resource to Thread A.

(The exact details of how Thread B returns the result to Thread A
are yet unclear to me. I imagine it could be done with std::future,
or even by never dispatching Thread B at all)

7. Thread A takes the raw data and creates a GLObject from it
by calling make_object_from_data(raw_data).

8. Thread A caches the newly created object into AsyncGLObjectPool
and returns a shared handle to it through the promise object.

9. Rendering System periodiacally (every frame) checks if any futures
have been fulfilled and if there are, retrieves them for later rendering.


Slightly inaccurate pic for demonstation:


       [check periodically]
RenderSystem --------> future<Resource>
    |                    ^
    | [request load]     | [make gl object, cache, and return handle]
    v                    |
AsyncGLObjectPool   AsyncGLObjectPool
    |                    ^
    | [request load]     | [make data resource, cache, and return handle]
    v                    |
AsyncDataPool        AsyncDataPool
    \                    /
     \      [load]      /
      \                /
       raw data on disk

*/

/*

AsyncDataPool works like an Active Object if the load is submitted
through AsyncDataPool::load_async(), so there's minimal blocking
on the calling thread.


Current implementation of AsyncDataPool primarily consists of these components:

- A single 'incoming request queue' that recieves requests through
  the calls to AsyncDataPool::load_async(). Part of Active Object.

- A single 'request handler' thread that dispatches the load requests
  based on the current state of the resource. Part of Active Object.

- A shared cache pool protected by a single mutex.
  Stores shared handles to the resources.

- A shared pending requests pool protected by a single mutex.
  Stores repeated load requests for the resource that's already being loaded.


For the purposes of concretely defining transactional logic of the
AsyncDataPool and clearly outlinig required synchronization points,
each resource in a shared pool is expected to exist in one
of the three possible states:

1. Not cached and not being loaded by another thread.
   This corresponds to the pool not having an entry for the resource
   (pool.find(resource_key) == pool.end()).

2. Not cached but is currently being loaded by another thread.
   This corresponds to the pool having a null entry for the resource
   (pool.at(resource_key) == nullptr).

   Additional load requests on this resource during this state
   are redirected to the 'pending requests pool', and will be
   fullfilled once the loading thread completes the load.
   An entry for the resource MUST NOT EXIST in the pending requests
   pool outside of the loading state. Failure to comply can lead
   to potentially 'leaking' the load requests.

3. Cached.
   This corresponds to the pool having a valid entry for the resource
   (pool.at(resource_key) is a valid shared handle to the resource).

*/



template<
    typename KeyT,
    typename ResourceT
>
class AsyncDataPool {
public:
    AsyncDataPool(ThreadPool& thread_pool);

    // Submits the requested resource for an asynchronous load and returns a future to it.
    auto load_async(auto&& path)
        -> Future<Shared<ResourceT>>;


    // Tries to load a cached value directly as Shared<ResourceT>.
    // Returns nullopt if an attempt to lock the cache pool failed
    // or if the requested resource is not in cache.
    //
    // Note: Succesfully retrieving the result from the future returned
    // by load_async() does not guarantee that the same resource
    // will be in cache right after.
    auto try_load_from_cache(const KeyT& file)
        -> std::optional<Shared<ResourceT>>;


    ~AsyncDataPool() noexcept;

private:
    std::unordered_map<KeyT, Shared<ResourceT>> pool_;
    mutable std::shared_mutex pool_mutex_;

    struct LoadRequest {
        KeyT file;
        Promise<Shared<ResourceT>> promise;
    };

    ThreadsafeQueue<LoadRequest> load_requests_;
    std::jthread load_request_handler_;

    // Could use some vector with SBO implementation.
    // A more suitable map could also be used.
    // But most likely it's not that big of a deal for performance
    // in the average use case.
    std::unordered_map<
        KeyT, std::vector<Promise<Shared<ResourceT>>>
    > pending_requests_;
    mutable std::mutex pending_requests_mutex_;

    // Used for synchronizing the destruction of the AsyncDataPool
    // until all loading threads complete their full routines.
    std::condition_variable_any destructor_cv_;
    // Loading thread counter synchronizes the destructor to block until
    // the number of active loading threads is zero.
    // Currently only modified under full write locks, so atomic is unnecessary.
    std::atomic<size_t> n_loading_threads_{ 0 };
    // Not going to try doing any fancy memory_order optimizations, not worth it.


    // TODO: Can be stored as shared_ptr<ThreadPool>,
    // will have to think about this. Might be unneccessary.
    ThreadPool& thread_pool_;

    void handle_load_requests(std::stop_token stoken);
    void handle_single_load_request(LoadRequest&& request);
    void fulfill_direct_load_request(LoadRequest request);


    // To be specialized externally for each ResourceT.
    Shared<ResourceT> load_data_from(const KeyT& file);
};




template<typename KeyT, typename ResourceT>
inline AsyncDataPool<KeyT, ResourceT>::
    AsyncDataPool(ThreadPool& thread_pool)
    : load_request_handler_{ [this](std::stop_token stoken) { handle_load_requests(stoken); } }
    , thread_pool_{ thread_pool }
{}



template<typename KeyT, typename ResourceT>
inline auto AsyncDataPool<KeyT, ResourceT>::
    load_async(auto&& path)
        -> Future<Shared<ResourceT>>
{
    auto [future, promise] = make_future_promise_pair<Shared<ResourceT>>();

    load_requests_.emplace(std::forward<decltype(path)>(path), std::move(promise));

    return future;
}


template<typename KeyT, typename ResourceT>
inline auto AsyncDataPool<KeyT, ResourceT>::
    try_load_from_cache(const KeyT& file)
        -> std::optional<Shared<ResourceT>>
{
    std::shared_lock lock{ pool_mutex_, std::try_to_lock };
    if (lock.owns_lock()) {
        auto it = pool_.find(file);
        if (it != pool_.end() && it->second) {
            return it->second;
        }
    }
    return std::nullopt;
}


template<typename KeyT, typename ResourceT>
inline AsyncDataPool<KeyT, ResourceT>::
    ~AsyncDataPool() noexcept
{
    std::unique_lock write_lock{ pool_mutex_ };
    destructor_cv_.wait(write_lock, [this] { return n_loading_threads_.load() == 0; });
}





template<typename KeyT, typename ResourceT>
inline void AsyncDataPool<KeyT, ResourceT>::
    handle_load_requests(std::stop_token stoken)
{
    while (true) {
        std::optional<LoadRequest> request =
            load_requests_.wait_and_pop(stoken);

        if (stoken.stop_requested()) { break; }

        assert(request.has_value());
        handle_single_load_request(std::move(request.value()));
    }
}


template<typename KeyT, typename ResourceT>
inline void AsyncDataPool<KeyT, ResourceT>::
    handle_single_load_request(LoadRequest&& request)
{
    // First we check for the common case of a resource
    // being already in the pool under a Read lock.
    {
        std::shared_lock read_lock{ pool_mutex_ };
        const auto& pool = pool_;

        auto it = pool.find(request.file);
        if (it != pool.end()) /* at(path) exists */ {
            if (it->second) /* at(path) is not null */ {
                set_result(std::move(request.promise), it->second);
                return;
            }
        }
    }
    // If that is not true, then we acquire Write locks
    // for the pool and for the set of pending requests
    // and recheck the state again because it could have
    // changed between releasing and acquiring the lock.
    //
    // Locking the pending requests mutex synchronizes with
    // any loading thread in order to avoid pushing a pending
    // request when the loading thread already finished resolving
    // all visible to it pending requests, which would effectively
    // 'leak' the request as a result.
    std::scoped_lock write_locks{ pool_mutex_, pending_requests_mutex_ };
    auto& pool = pool_;

    auto it = pool.find(request.file);
    if (it != pool.end()) /* at(path) exists */ {
        if (it->second) /* at(path) is not null */ {
            // Some other thread might have loaded the resource
            // while we were trying to reacquire the lock.
            set_result(std::move(request.promise), it->second);
            return;
        } else /* another thread is already loading the resource */ {
            // The loading thread will resolve all pending requests
            // once it's done loading.
            pending_requests_[std::move(request.file)].emplace_back(std::move(request.promise));
            return;
        }
    } else /* no resource found and no one is currently loading it */ {

        // Emplace a nullptr to signal that one thread is
        // already working on loading the resource.
        [[maybe_unused]]
        auto [it, was_emplaced] =
            pool.emplace(request.file, nullptr);

        assert(was_emplaced);

        // Increment the number of loading threads while the write lock
        // on the pool is still held. The AsyncDataPool will not be
        // destroyed until n_loading_threads_ is zero to prevent data
        // races from loading threads.

        n_loading_threads_.fetch_add(1);

        thread_pool_.emplace(
            &AsyncDataPool::fulfill_direct_load_request, this, std::move(request)
        );

    }
}


template<typename KeyT, typename ResourceT>
inline void AsyncDataPool<KeyT, ResourceT>::
    fulfill_direct_load_request(LoadRequest request)
{
    try {
        Shared<ResourceT> data{ load_data_from(request.file) };
        set_result(std::move(request.promise), data);

        // Lock the pending requests and resolve them.
        std::lock_guard pending_lock{ pending_requests_mutex_ };

        auto pending_it = pending_requests_.find(request.file);
        if (pending_it != pending_requests_.end()) {
            for (Promise<Shared<ResourceT>>& promise : pending_it->second) {
                set_result(std::move(promise), data);
            }
            pending_requests_.erase(pending_it);
        }

        // We cannot release the pending requests mutex yet because we haven't updated
        // the signalling nullptr value at(path), which can be misinterpreted
        // by the request handler thread as if we're still 'loading' the resource.
        // It would then push the another pending request here even though we are
        // already done processing them, therefore 'leaking' that request.
        //
        // So we hold onto the pending requests mutex until nullptr is updated.

        // Now acquire the Write lock on the pool and emplace the result.
        // This will have 2 mutexes locked by the same thread, but it should
        // be fine since any other loading thread will lock them in the same
        // order, and the request handler thread will lock them at the same time
        // with std::scoped_lock.
        std::unique_lock write_pool_lock{ pool_mutex_ };

        auto it = pool_.find(request.file);
        // The loading thread should find the pool entry in exactly this state:
        assert((it != pool_.end() && it->second == nullptr));

        it->second = std::move(data);

        // Only decrement loading thread count under a full write lock.
        // Release of a write lock serves as a synchronization with
        // the destructor of AsyncDataPool.
        n_loading_threads_.fetch_sub(1);
        // Also notify under a lock, else a data race can happen
        // between notification and destruction of cv (thanks, tsan).
        destructor_cv_.notify_one();

        // Done, release both locks and return.
    } catch (...) {
        set_exception(std::move(request.promise), std::current_exception());

        // Lock the pending requests and propagate the exception.
        std::lock_guard pending_lock{ pending_requests_mutex_ };

        auto pending_it = pending_requests_.find(request.file);
        if (pending_it != pending_requests_.end()) {
            for (Promise<Shared<ResourceT>>& promise : pending_it->second) {
                set_exception(std::move(promise), std::current_exception());
            }
            pending_requests_.erase(pending_it);
        }

        // Erase the entry from the pool - no longer loading.
        std::unique_lock write_pool_lock{ pool_mutex_ };

        auto it = pool_.find(request.file);

        assert((it != pool_.end() && it->second == nullptr));

        pool_.erase(it);

        n_loading_threads_.fetch_sub(1);
        destructor_cv_.notify_one();
    }
}











template<>
inline Shared<TextureData> AsyncDataPool<File, TextureData>::load_data_from(const File& file) {
    return std::make_shared<TextureData>(
        load_image_from_file<TextureData::pixel_type>(file)
    );
}



} // namespace josh
