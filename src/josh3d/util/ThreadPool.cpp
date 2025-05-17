#include "ThreadPool.hpp"
#include "ThreadName.hpp"


namespace josh {



ThreadPool::ThreadPool(size_t n_threads)
    : n_threads_    { n_threads }
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


void ThreadPool::execution_loop(
    std::stop_token stoken, // NOLINT: performance-*
    const size_t    thread_idx)
{
    const auto name = "pool worker " + std::to_string(thread_idx);
    set_current_thread_name(name.c_str());
    startup_latch_.arrive_and_wait();

    while (true) {

        // A single loop-around across all task queues
        // until a task fetch succedes or a loop ends.
        std::optional<task_type> task = try_fetch_or_steal(thread_idx);

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


auto ThreadPool::try_fetch_or_steal(size_t thread_idx)
    -> std::optional<task_type>
{
    std::optional<task_type> task{ std::nullopt };

    for (size_t offset{ 0 }; offset < n_threads(); ++offset) {
        const size_t idx{ (thread_idx + offset) % n_threads() };

        task = per_thread_tasks_[idx].try_lock_and_try_pop();

        if (task.has_value()) { break; }
    }

    return task;
}


void ThreadPool::drain_queue_until_empty(size_t thread_idx) {
    std::optional<task_type> task;
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
