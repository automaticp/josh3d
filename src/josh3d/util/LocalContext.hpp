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

    ~LocalContext() noexcept /* dare throwing in your tasks */ {
        if (task_counter_) {
            const auto sleep_budget = std::chrono::milliseconds(10);
            while (task_counter_->any_tasks_in_flight() || !tasks.empty()) {

                const auto wake_up_point =
                    std::chrono::steady_clock::now() + sleep_budget;

                while (std::optional task = tasks.try_pop()) {
                    (*task)();
                }

                std::this_thread::sleep_until(wake_up_point);
            }
        }
    }

private:
    TaskCounterGuard* task_counter_{ nullptr };
};



} // namespace josh
