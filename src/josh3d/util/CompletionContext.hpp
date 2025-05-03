#pragma once
#include "Coroutines.hpp"
#include "ThreadsafeQueue.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <list>
#include <optional>
#include <stop_token>
#include <thread>


namespace josh {


class CompletionContext {
public:
    // Maximum time a completion thread will sleep for per a single pass of current awaiters.
    std::atomic<std::chrono::nanoseconds> sleep_budget{ std::chrono::microseconds(100) };

    // Suspend until readyable becomes ready, then resume on the completion context.
    [[nodiscard]]
    auto until_ready(readyable auto&& readyable)
        -> awaiter auto;

    // Suspend until all readyable become ready, then resume on the completion context.
    [[nodiscard]]
    auto until_all_ready(std::ranges::range auto&& readyables)
        -> awaiter auto;

private:
    using await_job_type = Job<void>;

    struct NotReady {
        std::coroutine_handle<> awaiting_coroutine;     // We suspended from this. Will be resumed, once await_ready_job is done.
        std::coroutine_handle<> await_ready_job;        // Handle to the await_ready_job if it suspended.
        await_job_type          await_ready_job_owner_; // To keep the job from being destroyed when it completes.
    };

    [[nodiscard]]
    auto await_ready(
        readyable auto&&         readyable,
        std::coroutine_handle<>& out_self)
            -> await_job_type;

    [[nodiscard]]
    auto await_all_ready(
        std::ranges::range auto&& readyables,
        std::coroutine_handle<>&  out_self) // The lengths a man would go to manually resume a coroutine.
            -> await_job_type;

    ThreadsafeQueue<NotReady> requests_;
    std::jthread              completer_{ [this](std::stop_token stoken) { completer_loop(stoken); } };
    void completer_loop(std::stop_token stoken);

};




inline auto CompletionContext::await_all_ready(
    std::ranges::range auto&& readyables,
    std::coroutine_handle<>&  out_self)
        -> await_job_type
{
    // TODO: I am not sure about the lifetime of the range here.
    for (auto& readyable : readyables) {
        bool is_ready = false;
        do {
            is_ready = co_await if_not_ready(readyable, &out_self);
        } while (!is_ready);
    }
    co_return;
}


inline auto CompletionContext::await_ready(
    readyable auto&&         readyable,
    std::coroutine_handle<>& out_self)
        -> await_job_type
{
    while (!co_await if_not_ready(readyable, &out_self));
    co_return;
}


inline auto CompletionContext::until_all_ready(std::ranges::range auto&& readyables)
    -> awaiter auto
{
    using range_type = decltype(readyables);
    struct Awaiter {
        CompletionContext& self;
        range_type         readyables;

        std::coroutine_handle<>       await_ready_job       = nullptr;
        std::optional<await_job_type> await_ready_job_owner = std::nullopt;

        bool await_ready() {
            // Eagerly run the coroutine until completion or the first suspension.
            await_ready_job_owner = self.await_all_ready(readyables, await_ready_job);

            if (!await_ready_job) {
                // If the completion job never suspended, then await_ready_job is still null.
                // That means that all entries in the range were ready on the first pass,
                // and we don't have to suspend.
                return true;
            } else {
                // Otherwise, we will proceed to suspend to the completer context, which will
                // keep resuming the await_ready_job until it's done.
                return false;
            }
        }

        void await_suspend(std::coroutine_handle<> awaiting_coroutine) {
            self.requests_.emplace(awaiting_coroutine, await_ready_job, move_out(await_ready_job_owner));
        }

        void await_resume() const noexcept {}
    };
    return Awaiter{ *this, FORWARD(readyables) };
}


inline auto CompletionContext::until_ready(readyable auto&& readyable)
    -> awaiter auto
{
    using argument_type = decltype(readyable);
    struct Awaiter {
        CompletionContext& self;
        argument_type      readyable;

        std::coroutine_handle<>       await_ready_job       = nullptr;
        std::optional<await_job_type> await_ready_job_owner = std::nullopt;

        bool await_ready() {
            await_ready_job_owner = self.await_ready(readyable, await_ready_job);
            return !await_ready_job;
        }
        void await_suspend(std::coroutine_handle<> awaiting_coroutine) {
            self.requests_.emplace(awaiting_coroutine, await_ready_job, move_out(await_ready_job_owner));
        }
        void await_resume() const noexcept {}
    };
    return Awaiter{ *this, FORWARD(readyable) };
}


inline void CompletionContext::completer_loop(std::stop_token stoken) {

    std::list<NotReady> completables;

    auto loop_body = [&](std::chrono::nanoseconds sleep_budget) {

        auto loop_start    = std::chrono::steady_clock::now();
        auto wake_up_point = loop_start + sleep_budget;

        // Check the queue and emplace new completable.
        while (std::optional<NotReady> c = requests_.try_lock_and_try_pop()) {
            completables.emplace_back(move_out(c));
        }

        // Do a full sweep over all completable.
        auto it = completables.begin();
        while (it != completables.end()) {
            assert(!it->await_ready_job.done());

            // Resume the completion job again.
            it->await_ready_job.resume();

            // If it became done, then all of the awaitables are ready.
            if (it->await_ready_job.done()) {
                // Resume the awaiting_coroutine and erase the entry.
                // Hopefully, it just reschedules back to another context.
                it->awaiting_coroutine.resume();
                it = completables.erase(it);
            } else {
                ++it;
            }
        }

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
