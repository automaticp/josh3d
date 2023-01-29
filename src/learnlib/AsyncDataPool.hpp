#pragma once
#include "TextureData.hpp"
#include "Shared.hpp"
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
    return std::make_shared<TextureData>(StbImageData(path));
}





} // namespace learn
