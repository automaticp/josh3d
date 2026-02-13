#pragma once
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include "ContainerUtils.hpp"
#include "../CoroCore.hpp"
#include "Errors.hpp"
#include "Scalars.hpp"
#include "ScopeExit.hpp"
#include <cassert>
#include <concepts>
#include <coroutine>
#include <exception>
#include <utility>
#include <variant>


namespace josh::detail {


template<typename CoroT>
class GeneratorPromise
{
public:
    using coroutine_type = CoroT;
    using handle_type    = coroutine_type::handle_type;
    using result_type    = coroutine_type::result_type;

    // Begin boilerplate.

    auto get_return_object() -> coroutine_type { return coroutine_type(handle_type::from_promise(*this)); }

    auto initial_suspend() const noexcept -> std::suspend_always { return {}; }
    auto final_suspend()   const noexcept -> std::suspend_always { return {}; }

    void unhandled_exception() noexcept { result_ = std::current_exception(); }

    template<std::convertible_to<result_type> U>
    auto yield_value(U&& value) -> std::suspend_always { result_ = std::forward<U>(value); return {}; }

    void return_void() const noexcept {}

    // End boilerplate.

    bool has_any_result() const noexcept { return result_.index() != 0; }

    [[nodiscard]] auto extract_result() -> result_type
    {
        if      (is<result_type>       (result_)) return move_out<result_type>(result_);
        else if (is<std::exception_ptr>(result_)) std::rethrow_exception(move_out<std::exception_ptr>(result_));
        else safe_unreachable();
    }

private:
    struct no_result {};
    std::variant<no_result, result_type, std::exception_ptr> result_;
};


/*
A single atomic state that prevents setting a continuation
after ready signal has been issued.
*/
class ReadyAndContinuation
{
public:
    bool is_ready() const noexcept
    {
        return to_flag(packed.load(std::memory_order_acquire));
    }

    auto continuation() const noexcept -> std::coroutine_handle<>
    {
        return to_handle(packed.load(std::memory_order_acquire));
    }

    void wait_until_ready() const noexcept
    {
        while (true)
        {
            const uintptr old = packed.load(std::memory_order_acquire);
            if (to_flag(old)) return;
            // Can spuriously unblock after the continuation was set.
            // If the ready flag is not set though, we'll just wait again.
            packed.wait(old, std::memory_order_acquire);
        }
    }

    bool try_set_continuation(std::coroutine_handle<> handle) noexcept
    {
        // We expect that we are not ready, and the continuation is not set.
        uintptr expected = 0;

        // The continuation must be set only if we are not ready.
        // The operation does not change the ready state.
        const uintptr desired = to_packed(false, handle);

        const bool success =
            packed.compare_exchange_strong(expected, desired, std::memory_order_acq_rel);

        if (not success)
        {
            // This can fail for two reasons:
            //   1. Ready flag is already set;
            //   2. Continuation is already set.
            // Only the first one should be reported to the user.
            // The second one is a bug, and should be instead asserted.
            const uintptr observed = expected;

            if (to_flag(observed))
                return false;

            assert(not to_handle(observed) && "Setting a continuation when it was already set.");
            return false; // FIXME: Maybe terminate?
        }
        return true;
    }

    void became_ready() noexcept
    {
        const uintptr flag_mask = 1;
        const auto previous [[maybe_unused]] =
            packed.fetch_or(flag_mask, std::memory_order_release);
        assert(not (previous & flag_mask) && "Became ready twice.");
        packed.notify_all();
    }

private:
    std::atomic<uintptr> packed = 0;

    static auto to_handle(uintptr packed) noexcept -> std::coroutine_handle<>
    {
        const uintptr mask    = ~uintptr(1); // Wipe the lowest bit.
        void*         address = (void*)(packed & mask); // NOLINT(performance-*)
        return std::coroutine_handle<>::from_address(address);
    }

    static bool to_flag(uintptr packed) noexcept
    {
        const uintptr mask = uintptr(1); // Get the lowest bit.
        const bool    flag = bool(packed & mask);
        return flag;
    }

    static auto to_packed(bool flag, std::coroutine_handle<> handle) noexcept
        -> uintptr
    {
        const uintptr lowest_1      = uintptr(1);
        const uintptr address_value = uintptr(handle.address());
        const uintptr flag_value    = uintptr(flag & lowest_1);
        assert(!(address_value & lowest_1) && "Lowest bit already occupied. Cannot do magic packing.");
        return address_value | flag_value;
    }
};


template<typename CRTP, typename CoroT>
class JobPromiseCommon
{
public:
    using promise_type       = CRTP;
    using coroutine_type     = CoroT;
    using result_type        = coroutine_type::result_type;
    using handle_type        = std::coroutine_handle<promise_type>;
    using shared_handle_type = shared_coroutine_handle<promise_type>;

    auto get_return_object() -> coroutine_type
    {
        return coroutine_type(handle_type::from_promise(self()));
    }

    auto initial_suspend() const noexcept -> std::suspend_never
    {
        return {};
    }

    auto final_suspend() const noexcept -> awaiter<void, promise_type> auto
    {
        struct FinalAwaiter
        {
            bool await_ready() const noexcept { return false; }
            auto await_suspend(handle_type h) const noexcept
                -> std::coroutine_handle<>
            {
                promise_type& p = h.promise();
                // Release ownership *after* suspension at the end.
                //
                // If the Job was discarded by the calling side,
                // this will destroy the coroutine here.
                //
                // The return value is evaluated *before* destructors are run, so
                // we should be okay when transferring control to another coroutine.
                ON_SCOPE_EXIT([&]{ if (p.is_owning()) p.release_ownership(); });
                // Transfer control to "parent" coroutine, if any.
                if (std::coroutine_handle<> continuation = p.packed_state_.continuation())
                {
                    return continuation;
                }
                else
                {
                    return std::noop_coroutine();
                }
            }
            void await_resume() const noexcept {}
        };
        return FinalAwaiter{};
    }

    void unhandled_exception() noexcept
    {
        self().result_value() = std::current_exception();
        packed_state_.became_ready();
    }


    // Continuation (`co_await job` support).

    // If true, successfully set a continuation.
    // If false, the continuation could not be set because the job
    // became ready. You can read the result instead.
    //
    // No continuation must have been set previously.
    bool try_set_continuation(std::coroutine_handle<> handle) noexcept
    {
        return packed_state_.try_set_continuation(handle);
    }


    // Ready state.

    bool is_ready() const noexcept
    {
        const bool ready = packed_state_.is_ready();
        if constexpr (not_void<result_type>)
        {
            if (ready) { assert(self().has_result_value()); }
        }
        return ready;
    }

    void wait_for_result() const noexcept
    {
        packed_state_.wait_until_ready();
    }


    // Manual ownership handling.
    // TODO: Does this need to be public?

    bool is_owning() const noexcept
    {
        return bool(handle_);
    }

    void give_ownership(shared_handle_type handle) noexcept
    {
        assert(!handle_);
        handle_ = MOVE(handle);
    }

    auto release_ownership() noexcept -> shared_handle_type
    {
        assert(handle_);
        return MOVE(handle_);
    }

protected:
    // The promise will own the coroutine until final_suspend().
    // This is set through give_ownership() in the c-tor of the return object.
    shared_handle_type handle_ = handle_type(nullptr);

    // Atomically packed ready flag and continuation, if any.
    // This is to guarantee that continuation cannot be set after
    // the job becomes ready. If that could happen, the continuation
    // might have been skipped in the final_suspend().
    ReadyAndContinuation packed_state_{};

private:
    auto self()       noexcept ->       promise_type& { return static_cast<promise_type&>(*this); }
    auto self() const noexcept -> const promise_type& { return static_cast<const promise_type&>(*this); }
};


template<typename CoroT, typename ResultT>
class JobPromise
    : public JobPromiseCommon<JobPromise<CoroT, ResultT>, CoroT>
{
public:
    friend JobPromiseCommon<JobPromise<CoroT, ResultT>, CoroT>;
    using coroutine_type     = CoroT;
    using promise_type       = JobPromise;
    using result_type        = ResultT;
    using handle_type        = std::coroutine_handle<promise_type>;
    using shared_handle_type = shared_coroutine_handle<promise_type>;

    template<std::convertible_to<result_type> U>
    void return_value(U&& value) noexcept(noexcept(result_value() = FORWARD(value)))
    {
        assert(result_value().index() == 0);
        result_value() = FORWARD(value);
        this->packed_state_.became_ready();
    }

    auto get_result() -> result_type&
    {
        if      (is<result_type>       (result_value())) return get<result_type>(result_value());
        else if (is<std::exception_ptr>(result_value())) std::rethrow_exception(get<std::exception_ptr>(result_value()));
        else safe_unreachable();
    }

    // NOTE: This will make any following is_ready() call terminate because we assert that we have a value.
    // We need to make sure that this is only ever called before the Job is destroyed 100%, pinky promise.
    [[nodiscard]]
    auto extract_result() -> result_type
    {
        if      (is<result_type>       (result_value())) return move_out<result_type>(result_value());
        else if (is<std::exception_ptr>(result_value())) std::rethrow_exception(move_out<std::exception_ptr>(result_value()));
        else safe_unreachable();
    }


    struct no_result {};
    using result_value_type = std::variant<no_result, result_type, std::exception_ptr>;

    // Manual result handling.

    bool has_result_value() const noexcept
    {
        return result_value_.index() != 0;
    }

    // This can be emplaced externally. Does not signal readiness.
    template<std::convertible_to<result_value_type> U>
    void set_result_value(U&& value)
    {
        result_value() = FORWARD(value);
    }

    auto get_result_value() const -> const result_value_type&
    {
        return result_value();
    }

    // Take the result value away. Expects that readiness has *not* been signalled yet.
    [[nodiscard]]
    auto extract_result_value() -> result_value_type
    {
        assert(!this->is_ready());
        return move_out(result_value());
    }

private:
    result_value_type result_value_;
    auto result_value()       noexcept ->       result_value_type& { return result_value_; }
    auto result_value() const noexcept -> const result_value_type& { return result_value_; }
};


template<typename CoroT>
class JobPromise<CoroT, void>
    : public JobPromiseCommon<JobPromise<CoroT, void>, CoroT>
{
public:
    friend JobPromiseCommon<JobPromise<CoroT, void>, CoroT>;
    using result_type    = void;
    using coroutine_type = CoroT;
    using promise_type   = JobPromise;
    using handle_type    = std::coroutine_handle<promise_type>;
    using shared_handle  = shared_coroutine_handle<promise_type>;

    void return_void() noexcept
    {
        this->packed_state_.became_ready();
    }

    void get_result()
    {
        if (result_value()) std::rethrow_exception(result_value());
    }

    void extract_result()
    {
        if (result_value()) std::rethrow_exception(std::exchange(result_value(), std::exception_ptr(nullptr)));
    }

    using result_value_type = std::exception_ptr;

    // Manual result handling.

    bool has_result_value() const noexcept
    {
        return bool(result_value_);
    }

    // This can be emplaced externally. Does not signal readiness.
    template<std::convertible_to<result_value_type> U>
    void set_result_value(U&& value)
    {
        result_value() = FORWARD(value);
    }

    auto get_result_value() const -> const result_value_type&
    {
        return result_value();
    }

    // Take the result value away. Expects that readiness has *not* been signalled yet.
    [[nodiscard]]
    auto extract_result_value() -> result_value_type
    {
        assert(!this->is_ready());
        return std::exchange(result_value(), std::exception_ptr(nullptr));
    }

private:
    result_value_type result_value_;
    auto result_value()       noexcept ->       result_value_type& { return result_value_; }
    auto result_value() const noexcept -> const result_value_type& { return result_value_; }
};


} // namespace josh::detail

