#pragma once
#include "ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include <atomic>
#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <optional>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <future>
#include <latch>


namespace josh {



/*
"Do not write your own thread pool. Use an existing tasking system."

    - Sean Parent

"Also here's how to write a thread pool that has decent performance."

    - Sean Parent, paraphrased

And so I did...
*/


/*
A simple task-stealing thread pool with a minimal interface.
*/
class ThreadPool {
private:
    using task_t = UniqueFunction<void()>;

    size_t n_threads_;

    std::vector<ThreadsafeQueue<task_t>> per_thread_tasks_;

    std::vector<std::jthread> threads_;

    // Synchronizes beginning of the execution until all threads are ready;
    // Permits reading of shared data within execution_loop (like threads_.size())
    // without a potential of a data race.
    // Also shuts up tsan. Thanks, tsan, for telling me about this.
    std::latch startup_latch_;

    // Hints to where to emplace the next task. Updated with relaxed memory order.
    // No transaction concretely relies on the value read from this memory location.
    std::atomic<size_t> last_emplaced_idx_;

    // Number of times an emplace operation will loop-around all the task queues
    // trying to acquire a lock on one of them, until it settles on waiting on
    // one of the queues.
    // L(N) = min(max(512/N, 1), 64), where L is number of loops and N is number of threads (queues).
    // L(2) = 64; L(4) = 64; L(8) = 64; L(12) ~ 42; L(16) = 32; L(24) ~ 21; L(32) = 16; L(1024) = 1;
    size_t emplace_loops_;

public:
    // Initializes a thread pool with n_threads thread count. If the n_threads is not supplied,
    // uses a value returned by std::jthread::hardware_concurrency(), unless that value is 0,
    // in which case n_threads is set to 1.
    ThreadPool(size_t n_threads = default_thread_count());

    template<typename Fun, typename ...Args>
    auto emplace(Fun&& fun, Args&&... args)
        -> std::future<std::invoke_result_t<Fun, Args...>>;

    size_t n_threads() const noexcept { return n_threads_; }


    static size_t default_thread_count() noexcept {
        const size_t count{ std::jthread::hardware_concurrency() };
        return count ? count : 1;
    }


private:
    void execution_loop(std::stop_token stoken, const size_t thread_idx);

    std::optional<task_t> try_fetch_or_steal(size_t thread_idx);
    void drain_queue_until_empty(size_t thread_idx);

};




inline ThreadPool::ThreadPool(size_t n_threads)
    : n_threads_{ n_threads }
    , startup_latch_( n_threads + 1 )
    , emplace_loops_{ std::min(std::max(size_t{ 512 / n_threads }, size_t{ 1 }), size_t{ 64 }) }
{
    per_thread_tasks_.resize(n_threads);

    threads_.reserve(n_threads);
    for (size_t i{ 0 }; i < n_threads; ++i) {
        threads_.emplace_back(
            [this, thread_idx=i](std::stop_token stoken) { execution_loop(stoken, thread_idx); }
        );
    }

    startup_latch_.arrive_and_wait();
}




template<typename Fun, typename ...Args>
auto ThreadPool::emplace(Fun&& fun, Args&&... args)
    -> std::future<std::invoke_result_t<Fun, Args...>>
{
    using result_t = std::invoke_result_t<Fun, Args...>;

    std::promise<result_t> promise;
    auto future = promise.get_future();

    auto conforming_task = // of signature void()
        [promise=std::move(promise),
        fun=std::forward<Fun>(fun),
        ...args=std::forward<Args>(args)]() mutable
        {
            try {
                if constexpr (std::same_as<result_t, void>) {
                    std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)...);
                    promise.set_value();
                } else {
                    promise.set_value(
                        std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)...)
                    );
                }
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        };

    // Just a hint, relaxed should be fine (famous last words).
    size_t last_idx{ last_emplaced_idx_.load(std::memory_order_relaxed) };

    // Loop around the queues emplace_loops_ times and try emplacing
    // the task in whichever is not currently locked.
    bool was_emplaced;
    for (size_t i{ 0 }; i < (n_threads() * emplace_loops_); ++i) {
        last_idx = (last_idx + 1) % n_threads();

        was_emplaced =
            per_thread_tasks_[last_idx].try_emplace(std::move(conforming_task));

        if (was_emplaced) {
            last_emplaced_idx_.store(last_idx, std::memory_order_relaxed);
            break;
        }
    }

    // If all queues were locked, just sit patiently and wait until one of them unlocks.
    if (!was_emplaced) {
        per_thread_tasks_[last_idx].emplace(std::move(conforming_task));
        last_emplaced_idx_.store(last_idx, std::memory_order_relaxed);
    }

    return future;
}




inline void ThreadPool::execution_loop(std::stop_token stoken,
    const size_t thread_idx)
{
    startup_latch_.arrive_and_wait();

    while (true) {

        // A single loop-around across all task queues
        // until a task fetch succedes or a loop ends.
        std::optional<task_t> task = try_fetch_or_steal(thread_idx);

        if (task.has_value()) {
            std::invoke(task.value());
        } else /* no task has been fetched or stolen */ {
            // Sit in your queue until anything is pushed there.
            task = per_thread_tasks_[thread_idx].wait_and_pop(stoken);

            // Might still not have a valid task if stop was requested.
            if (task.has_value()) {
                std::invoke(task.value());
            } else /* stop requested */ {
                assert(stoken.stop_requested());
                // Retrieve all the tasks from this thread's queue until it's empty;
                // Will no longer do task stealing at this stage, but will finish
                // all the submitted tasks nonetheless.
                //
                // Task stealing can be implemented at this stage as well, but it's
                // a bit cumbersome to synchronize between all threads.
                drain_queue_until_empty(thread_idx);
                // Stop request happens only on termination of the ThreadPool itself
                // so it's 'safe' to assume that no new tasks will be submitted after.
                //
                // Note that the threads are not guaranteed to go into this non-stealing
                // mode right after a stop request. In fact, it's very likely most of
                // them will still be able to steal the majority of tasks remaining
                // before failing to fetch anything during a loop-around and falling through
                // to this mode of operation.
                break;
            }
        }
    }

}




inline auto ThreadPool::try_fetch_or_steal(size_t thread_idx)
    -> std::optional<task_t>
{
    std::optional<task_t> task{ std::nullopt };

    for (size_t offset{ 0 }; offset < n_threads(); ++offset) {
        const size_t idx{ (thread_idx + offset) % n_threads() };

        task = per_thread_tasks_[idx].try_lock_and_try_pop();

        if (task.has_value()) { break; }
    }

    return task;
}




inline void ThreadPool::drain_queue_until_empty(size_t thread_idx) {
    std::optional<task_t> task;
    while (true) {
        task = per_thread_tasks_[thread_idx].try_pop();
        if (task.has_value()) {
            std::invoke(task.value());
        } else {
            break;
        }
    }
}




} // namespace josh
