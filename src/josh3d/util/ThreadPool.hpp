#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "Ranges.hpp"
#include "Scalars.hpp"
#include "ThreadsafeQueue.hpp"
#include "Future.hpp"
#include "UniqueFunction.hpp"
#include <atomic>
#include <concepts>
#include <exception>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <latch>


namespace josh {


/*
A simple task-stealing thread pool with a minimal interface.

Based on Sean Parent's design from the talk "Better Code: Concurrency".
*/
class ThreadPool
{
public:
    // Initializes a thread pool with num_threads thread count. If the num_threads is not supplied,
    // uses a value returned by std::jthread::hardware_concurrency(), unless that value is 0,
    // in which case num_threads is set to 1.
    //
    // Optional `pool_name` can be provided which will tag the pool workers with it.
    // This is mostly used for debugging. The name should be short (<15 chars),
    // it will be truncated if it does not fit.
    ThreadPool(usize num_threads = default_thread_count(), String pool_name = {});

    // Submit a task and retrieve a Future to it.
    //
    // TODO: There's something to be said about classes that discard work
    // and go out of scope, when the work might operate on its members.
    // That's a fairly problematic scenario, and I wonder if a change of
    // interface could solve this. Maybe instead of Futures, we could return
    // certain "work tokens" that auto-block on destruction?
    template<typename Fun, typename ...Args>
    [[nodiscard]] auto emplace(Fun&& fun, Args&&... args)
        -> Future<std::invoke_result_t<Fun, Args...>>;

    auto num_threads() const noexcept -> usize { return num_threads_; }

    static auto default_thread_count() noexcept
        -> usize
    {
        const usize count = std::jthread::hardware_concurrency();
        return count ? count : 1;
    }

private:
    using task_type = UniqueFunction<void()>;
    String                             pool_name_;
    usize                              num_threads_;
    Vector<ThreadsafeQueue<task_type>> per_thread_tasks_;
    Vector<std::jthread>               threads_;

    // Synchronizes beginning of the execution until all threads are ready;
    // Permits reading of shared data within execution_loop (like threads_.size())
    // without a potential of a data race.
    // Also shuts up tsan. Thanks, tsan, for telling me about this.
    std::latch startup_latch_;

    // Hints to where to emplace the next task. Updated with relaxed memory order.
    // No transaction concretely relies on the value read from this memory location.
    std::atomic<usize> last_emplaced_idx_;

    // Number of times an emplace operation will loop-around all the task queues
    // trying to acquire a lock on one of them, until it settles on waiting on
    // one of the queues.
    // L(N) = min(max(512/N, 1), 64), where L is number of loops and N is number of threads (queues).
    // L(2) = 64; L(4) = 64; L(8) = 64; L(12) ~ 42; L(16) = 32; L(24) ~ 21; L(32) = 16; L(1024) = 1;
    usize emplace_loops_;

    void execution_loop(std::stop_token stoken, uindex thread_idx);
    auto try_fetch_or_steal(uindex thread_idx) -> Optional<task_type>;
    void drain_queue_until_empty(uindex thread_idx);

};


template<typename Fun, typename ...Args>
auto ThreadPool::emplace(Fun&& fun, Args&&... args)
    -> Future<std::invoke_result_t<Fun, Args...>>
{
    using result_type = std::invoke_result_t<Fun, Args...>;

    auto [future, promise] = make_future_promise_pair<result_type>();

    auto internal_task = // of signature void()
        [promise=MOVE(promise),
        fun=FORWARD(fun),
        ...args=FORWARD(args)]() mutable
    {
        try
        {
            if constexpr (std::same_as<result_type, void>)
            {
                fun(FORWARD(args)...);
                set_result(MOVE(promise));
            }
            else
            {
                set_result(MOVE(promise), fun(FORWARD(args)...));
            }
        }
        catch (...)
        {
            set_exception(MOVE(promise), std::current_exception());
        }
    };

    // Just a hint, relaxed should be fine (famous last words).
    uindex last_idx = last_emplaced_idx_.load(std::memory_order_relaxed);

    // Loop around the queues emplace_loops_ times and try emplacing
    // the task in whichever is not currently locked.
    bool was_emplaced = false;
    for (const uindex _ : irange(num_threads() * emplace_loops_))
    {
        last_idx = (last_idx + 1) % num_threads();

        was_emplaced =
            per_thread_tasks_[last_idx].try_emplace(MOVE(internal_task));

        if (was_emplaced)
        {
            last_emplaced_idx_.store(last_idx, std::memory_order_relaxed);
            break;
        }
    }

    // If all queues were locked, just sit patiently and wait until one of them unlocks.
    if (!was_emplaced)
    {
        per_thread_tasks_[last_idx].emplace(MOVE(internal_task));
        last_emplaced_idx_.store(last_idx, std::memory_order_relaxed);
    }

    // NOTE: Structured bindings do not auto-move here.
    return MOVE(future);
}


} // namespace josh
