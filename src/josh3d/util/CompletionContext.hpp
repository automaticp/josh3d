#pragma once
#include "CoroCore.hpp"
#include "Coroutines.hpp"
#include "ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <optional>
#include <ranges>
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
        -> awaiter<void> auto;

    // Suspend until all readyable become ready, then resume on the completion context.
    [[nodiscard]]
    auto until_all_ready(std::ranges::borrowed_range auto&& readyables)
        -> awaiter<void> auto;

    // Suspend until readyable becomes ready on the specified executor.
    //
    // Both the readiness check and resumption are guarateed to happen in the
    // context of the specified executor.
    [[nodiscard]]
    auto until_ready_on(executor auto& executor, readyable auto&& readyable)
        -> awaiter<void> auto;

private:
    using await_job_type = Job<>;

    struct NotReady {
        std::coroutine_handle<> awaiting_coroutine;     // We suspended from this. Will be resumed, once await_ready_job is done.
        std::coroutine_handle<> await_ready_job;        // Handle to the await_ready_job if it suspended.
        await_job_type          await_ready_job_owner_; // To keep the job from being destroyed when it completes.
    };

    using Task = UniqueFunction<void()>;

    using Request = std::variant<NotReady, Task>;

    ThreadsafeQueue<Request> requests_;
    std::jthread completer_{ [this](std::stop_token stoken) { completer_loop(stoken); } };
    void completer_loop(std::stop_token stoken);


    [[nodiscard]]
    auto _await_ready(
        readyable auto&&         readyable,
        std::coroutine_handle<>& out_self)
            -> await_job_type;

    [[nodiscard]]
    auto _await_all_ready(
        std::ranges::borrowed_range auto&& readyables,
        std::coroutine_handle<>&           out_self) // The lengths a man would go to manually resume a coroutine.
            -> await_job_type;

    void _resume_if_ready_on(
        executor auto&          executor,
        readyable auto&         readyable,
        std::coroutine_handle<> parent_coroutine);

};




auto CompletionContext::_await_all_ready(
    std::ranges::borrowed_range auto&& readyables,
    std::coroutine_handle<>&           out_self)
        -> await_job_type
{
    for (auto& readyable : readyables) {
        bool is_ready = false;
        do {
            is_ready = co_await if_not_ready(readyable, &out_self);
        } while (!is_ready);
    }
    co_return;
}


auto CompletionContext::_await_ready(
    readyable auto&&         readyable,
    std::coroutine_handle<>& out_self)
        -> await_job_type
{
    while (!co_await if_not_ready(readyable, &out_self));
    co_return;
}


void CompletionContext::_resume_if_ready_on(
    executor auto&           executor,
    readyable auto&          readyable,
    std::coroutine_handle<>  parent_coroutine)
{
    using cpo::is_ready;
    // NOTE: This is not a coroutine but just a task that the completion
    // context will run. This task just schedules another resume attempt.
    executor.emplace([&, parent_coroutine]() {
        if (is_ready(readyable)) {
            parent_coroutine.resume();
        } else {
            // Keep calling this until the readyable is ready.
            requests_.emplace([this, &executor, &readyable, parent_coroutine]() {
                _resume_if_ready_on(executor, readyable, parent_coroutine);
            });
        }
    });
}


template<executor E, readyable R>
auto CompletionContext::until_ready_on(E& executor, R&& readyable)
    -> awaiter<void> auto
{
    struct Awaiter {
        CompletionContext& self;
        E&                 executor;
        R                  readyable;

        // Always suspend, since we need to switch contexts.
        auto await_ready() const noexcept -> bool { return false; }
        void await_suspend(std::coroutine_handle<> parent_coroutine) {
            self._resume_if_ready_on(executor, readyable, parent_coroutine);
        }
        void await_resume() const noexcept {}
    };
    return Awaiter{ *this, executor, FORWARD(readyable) };
}


template<std::ranges::borrowed_range R>
auto CompletionContext::until_all_ready(R&& readyables)
    -> awaiter<void> auto
{
    struct Awaiter {
        CompletionContext& self;
        R                  readyables;

        std::coroutine_handle<>       await_ready_job       = nullptr;
        std::optional<await_job_type> await_ready_job_owner = std::nullopt;

        bool await_ready() {
            // Eagerly run the coroutine until completion or the first suspension.
            await_ready_job_owner = self._await_all_ready(readyables, await_ready_job);

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


template<readyable R>
auto CompletionContext::until_ready(R&& readyable)
    -> awaiter<void> auto
{
    struct Awaiter {
        CompletionContext& self;
        R                  readyable;

        std::coroutine_handle<>       await_ready_job       = nullptr;
        std::optional<await_job_type> await_ready_job_owner = std::nullopt;

        bool await_ready() {
            await_ready_job_owner = self._await_ready(readyable, await_ready_job);
            return !await_ready_job;
        }
        void await_suspend(std::coroutine_handle<> awaiting_coroutine) {
            self.requests_.emplace(awaiting_coroutine, await_ready_job, move_out(await_ready_job_owner));
        }
        void await_resume() const noexcept {}
    };
    return Awaiter{ *this, FORWARD(readyable) };
}


} // namespace josh
