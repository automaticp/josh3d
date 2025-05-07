#pragma once
#include "CategoryCasts.hpp"
#include "Semantics.hpp"
#include "ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include "TaskCounterGuard.hpp"
#include <chrono>
#include <thread>


namespace josh {


/*
Another weird utility for properly executing tasks in the local context of some enclosing class.

If the enclosing class wants to execute some operations "asynchronously" but *on the same thread*,
it *must* submit them to this local context. This is required to avoid deadlocking that could occur
when TaskCounterGuard's destructor waits for all tasks to complete by blocking the same thread
that is responsible for executing said tasks.

The enclosing class must periodically pull tasks from the queue and execute them.

If TaskCounterGuard is associated with the enclosing class (recommended),
this will spin and execute tasks on destruction until all tasks are complete.
*/
class LocalContext : public Immovable<LocalContext> {
public:
    ThreadsafeQueue<UniqueFunction<void()>> tasks;

    LocalContext() = default;
    LocalContext(TaskCounterGuard& task_counter) : task_counter_{ &task_counter } {}

    // Executor interface. Can be rescheduled to from a coroutine.
    void emplace(auto&& fun) { tasks.emplace(FORWARD(fun)); }

    // Execute the tasks in the queue until empty.
    // Returns the number of tasks executed.
    // Exceptions thrown by the underlying tasks are propagated.
    auto flush_strong()
        -> size_t
    {
        size_t n = 0;
        while (auto task = tasks.try_pop()) {
            (*task)();
            ++n;
        }
        return n;
    }

    // Execute the tasks in the queue until empty or the lock is contended.
    // Returns the number of tasks executed.
    // Exceptions thrown by the underlying tasks are propagated.
    auto flush_nonblocking()
        -> size_t
    {
        size_t n = 0;
        while (auto task = tasks.try_lock_and_try_pop()) {
            (*task)();
            ++n;
        }
        return n;
    }

    // Spin until the task queue is flushed *and* no more tasks are in flight.
    // Will simply do `flush_strong()` if no task counter is tracked.
    //
    // Returns the total number of tasks executed.
    //
    // Exceptions thrown by the underlying tasks are propagated.
    // The context is not guaranteed to be fully drained until the function
    // returns without throwing.
    auto drain_all_tasks(
        std::chrono::milliseconds sleep_budget = std::chrono::milliseconds(10))
            -> size_t
    {
        size_t n = 0;
        n += flush_strong();
        if (task_counter_) {
            size_t tasks_flushed = 0;
            // Here, new tasks can indeed be started from existing tasks, but
            // we make sure that we increment the count of child tasks before
            // decrementing the count of the parent tasks.
            while (task_counter_->any_tasks_in_flight() || tasks_flushed) {
                const auto wake_up_point =
                    std::chrono::steady_clock::now() + sleep_budget;

                n += flush_strong();

                std::this_thread::sleep_until(wake_up_point);
            }
        }
        return n;
    }

    // The destructor executes `drain_all_tasks()` but will swallow all
    // exceptions. Manually call `drain_all_tasks()` at the end of execution
    // if exception handling is desired.
    //
    // NOTE: You *very likely* want to drain the tasks yourself manually, because
    // while the tasks here are indeed drained forcefully, the objects those tasks
    // were referencing might already be destroyed at this point.
    ~LocalContext() noexcept {
        size_t tasks_drained = 0;
        do {
            try {
                tasks_drained = drain_all_tasks();
            } catch (...) { // NOLINT(bugprone-empty-catch)
                // TODO: Could we at least log instead of simply swallowing?
            }
        } while (tasks_drained != 0);
    }

private:
    TaskCounterGuard* task_counter_{ nullptr };
};



} // namespace josh
