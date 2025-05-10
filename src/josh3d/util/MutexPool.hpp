#pragma once
#include <atomic>
#include <cassert>
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
    auto new_mutex_ptr() noexcept
        -> mutex_type*
    {
        return &pool_[_next_index()];
    }

    // Get an index of *some* mutex in the pool.
    // The index refers to a valid mutex in the pool.
    [[nodiscard]]
    auto new_mutex_idx() noexcept
        -> size_t
    {
        return _next_index();
    }

    auto operator[](size_t mutex_idx) noexcept
        -> mutex_type&
    {
        assert(mutex_idx < pool_size());
        return pool_[mutex_idx];
    }

    auto operator[](size_t mutex_idx) const noexcept
        -> const mutex_type&
    {
        assert(mutex_idx < pool_size());
        return pool_[mutex_idx];
    }

private:
    std::vector<mutex_type> pool_;
    std::atomic<size_t>     index_ = 0;

    auto _next_index() noexcept
        -> size_t
    {
        return index_.fetch_add(1, std::memory_order_relaxed) % pool_size();
    }
};


} // namespace josh
