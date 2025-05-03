#pragma once
#include <atomic>
#include <vector>


namespace josh {


/*
A fixed-size pool of mutexes that can be used to significantly
reduce *average* contention where locking can happen "per-entry"
compared to central locking of the whole datastructure, but where
the naive alternative of storing a mutex for each entry is too expensive.
*/
template<typename MutexT>
class MutexPool {
public:
    using mutex_type = MutexT;

    MutexPool(size_t num_mutexes)
        : pool_(num_mutexes)
    {}

    auto pool_size() const noexcept
        -> size_t
    {
        return pool_.size();
    }

    // Get a pointer to *some* mutex from the pool.
    // The pointer is valid as long as the pool is alive.
    [[nodiscard]]
    auto new_mutex() noexcept
        -> mutex_type*
    {
        return &pool_[index_.fetch_add(1, std::memory_order_relaxed) % pool_size()];
    }

private:
    std::vector<mutex_type> pool_;
    std::atomic<size_t>     index_ = 0;
};


} // namespace josh
