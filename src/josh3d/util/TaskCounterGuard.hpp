#pragma once
#include "Semantics.hpp"
#include <atomic>
#include <cstddef>


namespace josh {


/*
Simple RAII counter for a number of "tasks-in-flight".

Blocks on destruction until all tasks have been reported as completed.

Useful when the tasks could potentially access resources
that have the same or wider lifetime than the object containing
the counter guard.
*/
class TaskCounterGuard : private Immovable<TaskCounterGuard> {
public:
    void report_task_started() {
        count_.fetch_add(1, std::memory_order_release); // Could be relaxed, I think.
    }

    void report_task_ended() {
        count_.fetch_sub(1, std::memory_order_release);
        count_.notify_one();
    }

    // Returns an RAII enabled guard that automatically increments/decrements the task counter.
    // Must be obtained on the same thread that would destroy the TaskCounterGuard,
    // before the operation is scheduled on another thread.
    //
    // Either use manual reporting, or this guard, not both.
    //
    // TODO: Only works for coroutines, since only they can switch context and
    // keep an immovable guard around.
    [[nodiscard]] auto obtain_task_guard() {
        class SingleTaskGuard : private Immovable<SingleTaskGuard> {
        public:
            SingleTaskGuard(TaskCounterGuard& self) : self_{ self } {
                self_.report_task_started();
            }
            ~SingleTaskGuard() noexcept {
                self_.report_task_ended();
            }
        private:
            TaskCounterGuard& self_;
        };
        return SingleTaskGuard{ *this };
    }

    ~TaskCounterGuard() noexcept {
        ptrdiff_t value = count_.load(std::memory_order_acquire);
        while (value > 0) {
            count_.wait(value, std::memory_order_acquire);
            // Count can no longer be incremented here, so
            // the load either acquires the same value as
            // the one compared with in the wait(),
            // or a value smaller than that.
            value = count_.load(std::memory_order_acquire);
        }
        assert(value == 0);
    }

private:
    std::atomic<ptrdiff_t> count_{ 0 };
};


} // namespace josh
