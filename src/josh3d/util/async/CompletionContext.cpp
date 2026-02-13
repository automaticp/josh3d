#include "CompletionContext.hpp"
#include "async/ThreadAttributes.hpp"
#include <list>


namespace josh {


void CompletionContext::completer_loop(std::stop_token stoken) {
    set_current_thread_name("completion ctx");

    std::list<NotReady> local_completables;
    std::vector<Task>   local_tasks;

    auto loop_body = [&](std::chrono::nanoseconds sleep_budget) {

        auto loop_start    = std::chrono::steady_clock::now();
        auto wake_up_point = loop_start + sleep_budget;

        // Check the queue and emplace new completable.
        while (std::optional<Request> request = requests_.try_lock_and_try_pop()) {
            const overloaded visitor = {
                [&](NotReady&& c) { local_completables.emplace_back(MOVE(c)); },
                [&](Task&&     t) { local_tasks       .emplace_back(MOVE(t)); },
            };
            visit(visitor, move_out(request));
        }

        // Do a full sweep over all completable.
        auto it = local_completables.begin();
        while (it != local_completables.end()) {
            assert(!it->await_ready_job.done());

            // Resume the completion job again.
            it->await_ready_job.resume();

            // If it became done, then all of the awaitables are ready.
            if (it->await_ready_job.done()) {
                // Resume the awaiting_coroutine and erase the entry.
                // Hopefully, it just reschedules back to another context.
                it->awaiting_coroutine.resume();
                it = local_completables.erase(it);
            } else {
                ++it;
            }
        }

        // Do a full sweep over all tasks.
        for (auto& task : local_tasks) {
            task();
        }
        local_tasks.clear();

        // Sleep for at max `sleep_budget` duration.
        // If the loop took longer than that, then we don't sleep at all.
        std::this_thread::sleep_until(wake_up_point);
    };


    while (!stoken.stop_requested()) {
        loop_body(sleep_budget.load());
    }


    // Drain the remaining completables that are still in the queue.
    // The queue no longer accepts new requests.
    while (!requests_.empty()) {
        // Use a fixed sleep_budget so that we accidentally
        // are not draining too slow or too fast.
        loop_body(std::chrono::microseconds(100));
    }
}


} // namespace josh
