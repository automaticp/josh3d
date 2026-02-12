#pragma once
#include "Semantics.hpp"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <utility>


namespace josh {


class SingleTaskGuard;


/*
Simple RAII counter for a number of "tasks-in-flight".

Blocks on destruction until all tasks have been reported as completed.

Useful when the tasks could potentially access resources
that have the same or wider lifetime than the object containing
the counter guard.
*/
class TaskCounterGuard
    : private Immovable<TaskCounterGuard>
{
public:
    void report_task_started() {
        count_.fetch_add(1, std::memory_order_release); // Could be relaxed, I think.
    }

    void report_task_ended() {
        count_.fetch_sub(1, std::memory_order_release);
        count_.notify_one();
    }

    // Returns true if at the time of query any tasks are still in flight.
    // Useful if you know that no new tasks can be started and want to spin until all are complete.
    auto any_tasks_in_flight() const noexcept
        -> bool
    {
        return count_.load(std::memory_order_acquire) != 0;
    }

    // Returns the number of tasks in flight at the point of the query.
    // WARN: Subject to TOCTOU and should be only used as a hint.
    auto hint_num_tasks_in_flight() const noexcept
        -> size_t
    {
        return count_.load(std::memory_order_acquire);
    }

    // Returns an RAII enabled guard that automatically increments/decrements the task counter.
    // Must be obtained on the same thread that would destroy the TaskCounterGuard,
    // before the operation is scheduled on another thread.
    //
    // Either use manual reporting, or this guard, not both.
    [[nodiscard]]
    auto obtain_task_guard()
        -> SingleTaskGuard;

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


class SingleTaskGuard
    : private MoveOnly<SingleTaskGuard>
{
public:
    SingleTaskGuard(TaskCounterGuard& guard)
        : guard_(&guard)
    {
        guard_->report_task_started();
    }
    SingleTaskGuard(SingleTaskGuard&& other) noexcept
        : guard_(std::exchange(other.guard_, nullptr))
    {}
    SingleTaskGuard& operator=(SingleTaskGuard&& other) noexcept {
        if (guard_) guard_->report_task_ended();
        guard_ = std::exchange(other.guard_, nullptr);
        return *this;
    }
    ~SingleTaskGuard() noexcept {
        if (guard_) guard_->report_task_ended();
    }
private:
    TaskCounterGuard* guard_;
};


[[nodiscard]]
inline auto TaskCounterGuard::obtain_task_guard()
    -> SingleTaskGuard
{
    return { *this };
}



} // namespace josh
