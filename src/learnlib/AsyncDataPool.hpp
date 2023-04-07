#pragma once
#include "TextureData.hpp"
#include "Shared.hpp"
#include "ThreadsafeQueue.hpp"
#include "ThreadPool.hpp"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <exception>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>
#include <queue>
#include <functional>
#include <variant>

namespace learn {



/*
(THIS COMMENT BLOCK IS OUTDATED)

There are, technically, two AsyncPools, one is for Data, and the other
is for GL Objects. Extra diffuculty arises because they have to work
together to transfer data from the hard drive, to the vram.

For example, a simple load request done by the rendering system
would have to go through both of the Pools:

1. Rendering System calls AsyncGLObjectPool::load(path) from Main Thread
to load a Resource (Texture/Model/etc.).

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
another output queue, or even by never dispatching Thread B at all)

7. Thread A takes the raw data and creates a GLObject from it
by calling make_object_from_data(raw_data).

8. Thread A caches the newly created object into AsyncGLObjectPool
and returns a shared handle to it by enqueing it into some
thread-safe OutQueue<Resource>.

9. Rendering System periodiacally (every frame) checks if
there are any resources avaliable in the OutQueue<Resource>
and if there are, retrieves them for later rendering.


Slightly inaccurate pic for dummies (like me):


        [process every frame]
RenderSystem --------> OutQueue<Resource>
    |                    ^
    | [request load]     | [make gl object and return handle]
    v                    |
AsyncGLObjectPool   AsyncGLObjectPool
    |                    ^
    | [request load]     | [make data resource and return handle]
    v                    |
AsyncDataPool        AsyncDataPool
    \                    /
     \      [load]      /
      \                /
       raw data on disk

Another thing is that the AsyncPool can act as an Active Object
in its public interface, so that the main thread would not
be blocked by the pool mutex.



Below is an approximate logic for an AsyncPool::load function,
that doesn't yet consider communication between AsyncGLObjectPool
and AsyncDataPool.

The return of the result is done by emplacing the result into a
result queue, that will later be checked by the interested party.
The way the result is returned might be changed, however, to whatever
fits best for a particular use case. Keep in mind the asynchrony.



pool.load(path, ...)
    |
lock the pool
    |
check if at(path) is already present
(find(path) != pool.end())
    |   \
    no   yes --> check if the shared pointer is null
    |                   |                   \
    |                  yes                  no --> enque the result and return
    |                   |
    |           then another thread is
    |           already loading the resource
    |                   |
    |           launch a task (thread/async)
    |           to wait until the resource is avaliable --> and return
    |                   |
    |                   v
    |           go to sleep (use condition variable)
    |           until some resource is avaliable
    |                   |
    |           if the avaliable resource is not the one
    |           that you were looking for, then go back to sleep,
    |           otherwise:
    |                   |
    |           lock the pool (will be locked from cv wakeup)
    |                   |
    |           copy the pointer from the pool --> enque the result and return
    |
    |
emplace a nullptr at(path) to signal that
the resource is alread being loaded by one thread
    |
launch a task (thread/async)
to load the resource from memory --> and return
    |
    v
load the resource (will block the thread)
    |
enque the result by copy of Shared<T>
(do as early as possible)
    |
lock the pool
    |
emplace (insert_or_assign) the result into the pool
by moving a local copy of Shared<T>
(assert that the previous value was null (can this fail the ABA test?))
    |
condition_variable.notify_all() --> and return

*/



template<typename ResourceT>
class AsyncDataPool {
private:
    std::unordered_map<std::string, Shared<ResourceT>> pool_;
    mutable std::shared_mutex pool_mutex_;

    struct LoadRequest {
        std::string path;
        std::promise<Shared<ResourceT>> promise;
    };

    ThreadsafeQueue<LoadRequest> load_requests_;
    std::jthread load_request_handler_;

    // Could use some vector with SBO implementation.
    // A more suitable map could also be used.
    // But most likely it's not that big of a deal for performance
    // in the average use case.
    std::unordered_map<
        std::string,
        std::vector<std::promise<Shared<ResourceT>>>
    > pending_requests_;
    mutable std::mutex pending_requests_mutex_;


    // TODO: Can be stored as shared_ptr<ThreadPool>,
    // will have to think about this. Might be unneccessary.
    ThreadPool& thread_pool_;

    void handle_load_requests(std::stop_token stoken) {
        while (true) {
            std::optional<LoadRequest> request =
                load_requests_.wait_and_pop(stoken);

            if (stoken.stop_requested()) { break; }

            assert(request.has_value());
            handle_single_load_request(std::move(request.value()));
        }
    }

    void handle_single_load_request(LoadRequest&& request) {
        // First we check for the common case of a resource
        // being already in the pool under a Read lock.
        {
            std::shared_lock read_lock{ pool_mutex_ };
            const auto& pool = pool_;

            auto it = pool.find(request.path);
            if (it != pool.end()) /* at(path) exists */ {
                if (it->second) /* at(path) is not null */ {
                    request.promise.set_value(it->second);
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

        auto it = pool.find(request.path);
        if (it != pool.end()) /* at(path) exists */ {
            if (it->second) /* at(path) is not null */ {
                // Some other thread might have loaded the resource
                // while we were trying to reacquire the lock.
                request.promise.set_value(it->second);
                return;
            } else /* another thread is already loading the resource */ {
                // The loading thread will resolve all pending requests
                // once it's done loading.
                pending_requests_[std::move(request.path)].emplace_back(std::move(request.promise));
                return;
            }
        } else /* no resource found and no one is currently loading it */ {

            // Emplace a nullptr to signal that one thread is
            // already working on loading the resource.
            [[maybe_unused]]
            auto [it, was_emplaced] =
                pool.emplace(request.path, nullptr);

            assert(was_emplaced);

            // FIXME: The thread dispatched by the ThreadPool can try to access
            // an already destroyed AsyncDataPool. We need a way to defer destruction
            // until all submitted load requests are complete.
            //
            // Might collect futures in a std::vector<std::future<void(LoadRequest)>>
            // and wait on all of them in the destructor.
            //
            // Will have to think about it a bit more.

            thread_pool_.emplace(
                [this](LoadRequest request) {
                    try {
                        Shared<ResourceT> data{ load_data_from(request.path) };
                        request.promise.set_value(data);

                        // Lock the pending requests and resolve them.
                        std::lock_guard pending_lock{ pending_requests_mutex_ };

                        auto pending_it = pending_requests_.find(request.path);
                        if (pending_it != pending_requests_.end()) {
                            for (std::promise<Shared<ResourceT>>& promise : pending_it->second) {
                                promise.set_value(data);
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

                        auto it = pool_.find(request.path);
                        // The loading thread should find the pool entry in exactly this state:
                        assert((it != pool_.end() && it->second == nullptr));

                        it->second = std::move(data);

                        // Done, release both locks and return.
                    } catch (...) {
                        request.promise.set_exception(std::current_exception());

                        // Lock the pending requests and propagate the exception.
                        std::lock_guard pending_lock{ pending_requests_mutex_ };

                        auto pending_it = pending_requests_.find(request.path);
                        if (pending_it != pending_requests_.end()) {
                            for (std::promise<Shared<ResourceT>>& promise : pending_it->second) {
                                promise.set_exception(std::current_exception());
                            }
                            pending_requests_.erase(pending_it);
                        }

                        // Erase the entry from the pool - no longer loading.
                        std::unique_lock write_pool_lock{ pool_mutex_ };

                        auto it = pool_.find(request.path);

                        assert((it != pool_.end() && it->second == nullptr));

                        pool_.erase(it);

                    }
                },
                std::move(request)
            );

        }
    }

public:
    AsyncDataPool(ThreadPool& thread_pool)
        : load_request_handler_{ &AsyncDataPool::handle_load_requests, *this }
        , thread_pool_{ thread_pool }
    {}

    // Submits the requested resource for an asynchronous load and returns a future to it.
    std::future<Shared<ResourceT>> load_async(auto&& path) {

        std::promise<Shared<ResourceT>> promise;
        auto future = promise.get_future();

        load_requests_.emplace(std::forward<decltype(path)>(path), std::move(promise));

        return future;
    }

    // Tries to load a cached value directly as Shared<ResourceT>.
    // Returns nullopt if an attempt to lock the cache pool failed
    // or if the requested resource is not in cache.
    std::optional<Shared<ResourceT>> try_load_from_cache(const std::string& path) {
        std::shared_lock lock{ pool_mutex_, std::try_to_lock };
        if (lock.owns_lock()) {
            auto it = pool_.find(path);
            if (it != pool_.end() && it->second) {
                return it->second;
            }
        }
        return std::nullopt;
    }

private:
    // To be specialized externally for each ResourceT.
    Shared<ResourceT> load_data_from(const std::string& path);
};




template<>
inline Shared<TextureData> AsyncDataPool<TextureData>::load_data_from(const std::string& path) {
    return std::make_shared<TextureData>(TextureData::from_file(path));
}



} // namespace learn
