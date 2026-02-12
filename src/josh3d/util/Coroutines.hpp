#pragma once
#include "CoroCore.hpp"
#include "detail/Coroutines.hpp"
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include "Semantics.hpp"
#include <cassert>
#include <coroutine>
#include <optional>
#include <type_traits>


namespace josh {


/*
NOTE: Not tested or used anywhere. Might work?
*/
template<typename T>
class Generator {
public:
    using result_type        = T;
    using value_type         = T;
    using promise_type       = detail::GeneratorPromise<Generator>;
    using handle_type        = std::coroutine_handle<promise_type>;
    using unique_handle_type = unique_coroutine_handle<promise_type>;

    Generator(handle_type handle) : handle_{ handle } {}

    auto operator()()
        -> std::optional<result_type>
    {
        handle_type   h = handle_.get();
        promise_type& p = h.promise();
        if (!h.done()) {
            h.resume(); // UB if h.done(), so guard.
            // Check again, because we might have `co_return`ed after last resume().
            if (!h.done()) {
                return { p.extract_result() };
            }
        }
        return std::nullopt;
    }

private:
    unique_handle_type handle_;
};




/*
A flavour of the eager task that preserves it's lifetime as long as the
task is running.

Think of this as a future/promise pair augmentend with the actual
task attached to the promise.

The Job is a bridge from the coroutine world to the normal execution,
and is also a barrier that stops the coroutine-plague from propagating
through your whole codebase (cough-cough, lazy tasks...).

It's a bit more heavyweight than a lazy task, so adjust your usage accordingly.
*/
template<typename T = void>
class Job
    : private MoveOnly<Job<T>>
{
public:
    using result_type        = T;
    using result_ref_type    = std::conditional_t<not_void<result_type>, std::add_lvalue_reference_t<result_type>, void>;
    using result_cref_type   = std::conditional_t<not_void<result_type>, std::add_lvalue_reference_t<std::add_const_t<result_type>>, void>;
    using value_type         = T;
    using promise_type       = detail::JobPromise<Job, result_type>;
    using handle_type        = std::coroutine_handle<promise_type>;
    using shared_handle_type = shared_coroutine_handle<promise_type>;

    // The Job will eagerly start on the current thread, then
    // an explicit rescheduling can be done by the coroutine.
    explicit Job(handle_type handle)
        : handle_{ handle }
    {
        handle_.get().promise().give_ownership(handle_);
    }

    // Is the task completed. Synchronized by an atomic.
    // If `true`, retrieving the result will not block.
    auto is_ready() const noexcept
        -> bool
    {
        return handle_ && handle_.get().promise().is_ready();
    }

    // Block until the job has finished.
    void wait_until_ready() const noexcept {
        assert(handle_);
        promise_type& p = handle_.get().promise();
        p.wait_for_result();
    }

    // Obtain the result or raise an exception.
    auto get_result() const&
        -> result_ref_type
    {
        assert(handle_);
        promise_type& p = handle_.get().promise();
        p.wait_for_result();
        return p.get_result();
    }

    auto get_result() &&
        -> result_type
    {
        assert(handle_);
        promise_type& p = handle_.get().promise();
        p.wait_for_result();
        return p.extract_result();
    }

    auto operator co_await() const& noexcept
        -> awaiter<result_ref_type> auto
    {
        struct Awaiter {
            const Job& self;
            auto await_ready() const noexcept -> bool { return self.is_ready(); }
            auto await_suspend(std::coroutine_handle<> parent) const noexcept
                -> std::coroutine_handle<>
            {
                if (self.handle_.get().promise().try_set_continuation(parent)) {
                    // If we succesfully set ourselves as a continuation, then we
                    // shouldn't resume anything.
                    //
                    // NOTE: I think noop coroutine is required, we cannot just
                    // return a null coroutine handle.
                    return std::noop_coroutine();
                } else {
                    // Otherwise, the try_set_continuation() failed because
                    // the job became ready. We can resume ourselves instead,
                    // as the result is now available.
                    return parent;
                }
            }
            auto await_resume() const -> result_ref_type { return self.get_result(); }
        };
        return Awaiter{ *this };
    }

    auto operator co_await() && noexcept
        -> awaiter<result_type> auto
    {
        struct Awaiter {
            Job&& self;
            auto await_ready() const noexcept -> bool { return self.is_ready(); }
            auto await_suspend(std::coroutine_handle<> parent) const noexcept
                -> std::coroutine_handle<>
            {
                if (self.handle_.get().promise().try_set_continuation(parent)) {
                    return std::noop_coroutine();
                } else {
                    return parent;
                }
            }
            auto await_resume() const -> result_type { return MOVE(self).get_result(); }
        };
        return Awaiter{ MOVE(*this) };
    }

private:
    shared_handle_type handle_;
    template<typename> friend class SharedJob;
};


/*
Schedule a function to run as a job on the specified executor.

Note that the function arguments are forwarded without copies.
Beware that normal functions are usually reference-hungry, and
do not materialize its arguments. Mind the lifetimes of passed
arguments, or wrap the function in a lambda with proper captures.
*/
template<typename Func, typename ...Args>
auto launch_job_on(
    executor auto& e,
    Func&&         f,
    Args&&...      args)
        -> Job<std::invoke_result_t<Func, Args...>>
{
    co_await reschedule_to(e);
    co_return FORWARD(f)(FORWARD(args)...);
}


/*
This is not a coroutine itself, but is a shared awaitable handle to a JobPromise.
*/
template<typename T = void>
class SharedJob
    : private Copyable<SharedJob<T>>
{
public:
    using result_type      = Job<T>::result_type;
    using result_ref_type  = Job<T>::result_ref_type;
    using result_cref_type = Job<T>::result_cref_type;
    using value_type       = Job<T>::value_type;

    SharedJob(Job<T>&& job) : handle_{ MOVE(job.handle_) } {}

    // Is the task completed. Synchronized by an atomic.
    // If `true`, retrieving the result will not block.
    auto is_ready() const noexcept
        -> bool
    {
        return handle_ && handle_.get().promise().is_ready();
    }

    // Block until the job has finished.
    void wait_until_ready() const noexcept {
        assert(handle_);
        typename Job<T>::promise_type& p = handle_.get().promise();
        p.wait_for_result();
    }

    auto get_result() const
        -> result_cref_type
    {
        assert(handle_);
        typename Job<T>::promise_type& p = handle_.get().promise();
        p.wait_for_result();
        return p.get_result();
    }

    [[deprecated("Needs nonblocking list-of-waiters implementation. Not this garbage.")]]
    auto operator co_await() const noexcept
        -> awaiter<result_cref_type> auto
    {
        struct Awaiter {
            const SharedJob& self;
            bool await_ready() const noexcept { return self.is_ready(); }
            bool await_suspend(std::coroutine_handle<>) const noexcept { return !self.is_ready(); }
            auto await_resume() const -> result_cref_type { return self.get_result(); }
        };
        return Awaiter{ *this };
    }


private:
    Job<T>::shared_handle_type handle_;
};


} // namespace josh
