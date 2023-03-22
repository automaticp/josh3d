#pragma once
#include "TextureData.hpp"
#include "Shared.hpp"
#include <cassert>
#include <condition_variable>
#include <memory>
#include <stop_token>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <future>
#include <queue>
#include <functional>

namespace learn {


// WIP, DO NOT TOUCH!


/*

Shared<T> data = pool.try_load_existing(path);

std::future<Shared<T>> data_future;

if (!data) {
    data_future = pool.force_load_async(path);
}

// ...

// Try getting it at a later time.
// Perhaps repeatedly...

if (data_future.valid()) {
    data = data_future.get();
}

// Or block until ready.

data = data_future.get();

*/


// This is the most anti-DOD thing I could come up with.
template<typename T>
class AsyncResource {
private:
    Shared<T> data_;
    std::future<Shared<T>> future_;

};

// WIP, DO NOT TOUCH!
template<typename T>
class AsyncDataPool {
private:
    using pool_t = std::unordered_map<std::string, Shared<T>>;
    pool_t pool_;

    mutable std::mutex mutex_;

public:
    Shared<T> load(const std::string& path);

    std::future<Shared<T>> load_async(const std::string& path);

    void clear();
    void clear_unused();

private:
    Shared<T> force_load(const std::string& path);
    std::future<Shared<T>> force_load_async(const std::string& path);

    // To be specialized externally
    Shared<T> load_data_from(const std::string& path);

};

template<typename T>
std::future<Shared<T>> AsyncDataPool<T>::load_async(const std::string& path) {

    // Lock the pool and search for an existing resource.
    {
        std::lock_guard lock{ mutex_ };

        auto it = pool_.find(path);
        if (it != pool_.end()) {
            Shared<T> copy{ it->second };
            // FIXME: You can release the lock here

            // HOW TO MAKE A FUTURE FROM A NORMAL VALUE???
            std::promise<Shared<T>> promise;
            promise.set_value(std::move(copy));
            return promise.get_future();
            // PROMISE IS DESTROYED HERE, WHAT NOW???
            // IS THIS FINE??? CONCURRENCY GODS HELP ME
        }
    }

    // If not found, asynchronously load from file.
    return force_load_async(path);
}

template<typename T>
std::future<Shared<T>> AsyncDataPool<T>::force_load_async(const std::string& path) {
    return std::async(
        std::launch::async,
        &force_load, this, path
    );
}

template<typename T>
Shared<T> AsyncDataPool<T>::force_load(const std::string& path) {
    Shared<T> new_data = load_data_from(path);

    std::lock_guard lock{ mutex_ };
    auto [it, was_emplaced] = pool_.emplace(path, std::move(new_data));
    return it->second;
}


template<typename T>
Shared<T> AsyncDataPool<T>::load(const std::string& path) {

    // Lock the pool and search for an existing resource.
    {
        std::lock_guard lock{ mutex_ };

        auto it = pool_.find(path);
        if (it != pool_.end()) {
            return it->second;
        }
    }

    // If not found, synchronously load from file.
    return force_load(path);
}

template<typename T>
void AsyncDataPool<T>::clear() {
    std::lock_guard lock{ mutex_ };
    pool_.clear();
}

template<typename T>
void AsyncDataPool<T>::clear_unused() {
    std::lock_guard lock{ mutex_ };
    for (auto it = pool_.begin(); it != pool_.end();) {
        if ( it->second && it->second.use_count() == 1u ) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }
}



template<>
inline Shared<TextureData> AsyncDataPool<TextureData>::load_data_from(const std::string& path) {
    return std::make_shared<TextureData>(TextureData::from_file(path));
}






/*
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


// Can have other info attached, like entity id
template<typename T>
struct LoadResult {
    std::string path;
    Shared<T> resource;
};

// FOR DEMONSTRATION PURPOSES,
// USE A BETTER IMPLEMENTATION!
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mtx_;

public:
    void push(T element) {
        std::lock_guard lk{ mtx_ };
        queue_.push(std::move(element));
    }


    // What's below does not work,
    // as a 'check empty and pop' combo is not atomic.
    // Just assume that the recieving end of the queue
    // is not implemented.
    T pop() {
        std::lock_guard lk{ mtx_ };
        T value{ std::move(queue_.front()) };
        queue_.pop();
        return value;
    }

    bool empty() const noexcept {
        std::lock_guard lk{ mtx_ };
        return queue_.empty();
    }

};

template<typename ResourceT>
using OutQueue = ThreadSafeQueue<LoadResult<ResourceT>>;

// Some output queue that recieves the results
// and is consumed later by the interested party.
template<typename ResourceT>
inline OutQueue<ResourceT> out_queue;

template<typename ResourceT>
class AsyncDataPool2 {
private:
    std::unordered_map<std::string, Shared<ResourceT>> pool_;

    mutable std::mutex mutex_;
    // This could be a map: string -> condition_variabe
    // So that threads waiting on a particular resource
    // would not be notified only when their resource is ready.
    std::condition_variable cv_;

public:
    void load(const std::string& path) {
        // WELCOME TO PAIN TOWN
        std::lock_guard lk{ mutex_ };

        auto it = pool_.find(path);
        if (it != pool_.end()) /* some entry at(path) exists */ {

            if (it->second) /* Shared<T> is not null */ {

                out_queue<ResourceT>.push({ path, it->second });
                return;

            } else /* another thread is loading the resource */ {

                // FIXME: Use thread pool.
                std::jthread waiting_thread{
                    [this](const std::string& path) {

                        std::unique_lock lk{ mutex_ };
                        typename decltype(pool_)::iterator it;
                        cv_.wait(lk, [&] {
                            it = pool_.find(path);
                            return it != pool_.end()
                                && it->second;
                        });
                        out_queue<ResourceT>.push({ path, it->second });

                    },
                    // std::unordered_map guarantees that references
                    // to keys/values are not invalidated.
                    std::cref(it->first)
                };
                waiting_thread.detach();
                return;

            }

        } else /* no resource found and no one is currently loading it */ {

            [[maybe_unused]]
            auto [it, was_emplaced] =
                pool_.emplace(path, Shared<ResourceT>());

            // FIXME: Use thread pool.
            std::jthread loading_thread{
                [&, this](const std::string& path) {

                    Shared<ResourceT> data = load_data_from(path);
                    out_queue<ResourceT>.push({ path, data });

                    std::lock_guard lk{ mutex_ };

                    assert(!pool_.at(path));
                    pool_.insert_or_assign(path, std::move(data));

                    cv_.notify_all();
                },
                std::cref(path)
            };
        }





    }

    // To be specialized externally for each ResourceT.
    Shared<ResourceT> load_data_from(const std::string& path);
};





} // namespace learn
