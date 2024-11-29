#pragma once
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include "ContainerUtils.hpp"
#include "Semantics.hpp"
#include <atomic>
#include <cassert>
#include <concepts>
#include <coroutine>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>


namespace josh {


template<typename T, typename ResultT = void, typename PromiseT = void>
concept awaiter = requires(T awaiter, std::coroutine_handle<PromiseT> coroutine) {
    { awaiter.await_ready()            } -> std::convertible_to<bool>;
    { awaiter.await_suspend(coroutine) } -> any_of<void, bool, std::coroutine_handle<PromiseT>>;
    { awaiter.await_resume()           } -> std::same_as<ResultT>;
};


template<typename T, typename ResultT = void, typename PromiseT = void>
concept awaitable =
    awaiter<T, ResultT, PromiseT> ||
    requires(T awaitable) { { awaitable.operator co_await() } -> awaiter<ResultT, PromiseT>; } ||
    requires(T awaitable) { { operator co_await(awaitable)  } -> awaiter<ResultT, PromiseT>; };




template<typename T = void>
class unique_coroutine_handle {
public:
    unique_coroutine_handle(std::coroutine_handle<T> handle) noexcept
        : handle_{ handle }
    {}

    auto operator->() const noexcept -> std::coroutine_handle<T>* { return &handle_; }
    auto operator*()  const noexcept -> std::coroutine_handle<T>& { return handle_;  }
    auto get()        const noexcept -> std::coroutine_handle<T>  { return handle_;  }

    explicit operator bool() const noexcept { return bool(handle_); }
    bool valid()             const noexcept { return bool(handle_); }

    operator unique_coroutine_handle<>() && noexcept { return { std::exchange(handle_, nullptr) }; }


    unique_coroutine_handle(const unique_coroutine_handle&)           = delete;
    unique_coroutine_handle operator=(const unique_coroutine_handle&) = delete;

    unique_coroutine_handle(unique_coroutine_handle&& other) noexcept
        : handle_{ std::exchange(other.handle_, nullptr) }
    {}

    unique_coroutine_handle& operator=(unique_coroutine_handle&& other) noexcept {
        handle_ = std::exchange(other.handle_, nullptr);
        return *this;
    }

    ~unique_coroutine_handle() noexcept { if (handle_) { handle_.destroy(); } }

private:
    std::coroutine_handle<T> handle_;
    template<typename U> friend class shared_coroutine_handle;
};




template<typename T = void>
class shared_coroutine_handle {
public:
    shared_coroutine_handle(std::coroutine_handle<T> handle) noexcept
        : handle_  { handle               }
        , refcount_{ new refcount_type(1) }
    {}

    auto operator->() const noexcept -> const std::coroutine_handle<T>* { return &handle_; }
    auto operator*()  const noexcept -> const std::coroutine_handle<T>& { return handle_;  }
    auto get()        const noexcept ->       std::coroutine_handle<T>  { return handle_;  }

    explicit operator bool() const noexcept { return bool(handle_); }
    bool valid()             const noexcept { return bool(handle_); }
    bool only_owner()        const noexcept { return refcount_ && refcount_->load(std::memory_order_acquire); }
    auto use_count_hint()    const noexcept { return refcount_ ? refcount_->load(std::memory_order_relaxed) : 0; }

    operator shared_coroutine_handle<>() && noexcept {
        return { std::exchange(handle_, nullptr) };
    }

    operator shared_coroutine_handle<>() const& noexcept {
        acquire_ownership();
        return { handle_, refcount_, shared_coroutine_handle<>::PrivateKey{} };
    }

    // Copy ctor.
    shared_coroutine_handle(const shared_coroutine_handle& other) noexcept
        : handle_  { other.handle_   }
        , refcount_{ other.refcount_ }
    {
        acquire_ownership();
    }

    // Move ctor.
    shared_coroutine_handle(shared_coroutine_handle&& other) noexcept
        : handle_  { std::exchange(other.handle_,   nullptr) }
        , refcount_{ std::exchange(other.refcount_, nullptr) }
    {}

    // Converting unique -> shared ctor.
    shared_coroutine_handle(unique_coroutine_handle<T>&& other) noexcept
        : shared_coroutine_handle{ std::exchange(other.handle_, nullptr), PrivateKey{} }
    {}

    // Copy assignment.
    auto operator=(const shared_coroutine_handle& other) noexcept
        -> shared_coroutine_handle&
    {
        if (this != &other) {
            release_ownership();
            handle_   = other.handle_;
            refcount_ = other.refcount_;
            acquire_ownership();
        }
        return *this;
    }

    // Move assignment.
    auto operator=(shared_coroutine_handle&& other) noexcept
        -> shared_coroutine_handle&
    {
        release_ownership();
        handle_   = std::exchange(other.handle_,   nullptr);
        refcount_ = std::exchange(other.refcount_, nullptr);
        return *this;
    }

    // Converting unique -> shared assignment.
    auto operator=(unique_coroutine_handle<T>&& other) noexcept
        -> shared_coroutine_handle&
    {
        release_ownership();
        handle_   = std::exchange(other.handle_, nullptr);
        refcount_ = new refcount_type(1);
        return *this;
    }


    ~shared_coroutine_handle() noexcept { release_ownership(); }

private:
    using refcount_type = std::atomic<size_t>;

    std::coroutine_handle<T> handle_;
    refcount_type*           refcount_;

    struct PrivateKey {};

    template<typename U> friend class shared_coroutine_handle;

    shared_coroutine_handle(std::coroutine_handle<T> handle, refcount_type* refcount, PrivateKey)
        : handle_  { handle   }
        , refcount_{ refcount }
    {}

    shared_coroutine_handle(std::coroutine_handle<T> handle, PrivateKey)
        : handle_  { handle               }
        , refcount_{ new refcount_type(1) }
    {}

    void acquire_ownership() const noexcept {
        if (refcount_) {
            refcount_->fetch_add(1, std::memory_order_relaxed);
        }
    }

    void release_ownership() noexcept {
        if (refcount_ &&
            refcount_->fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete refcount_;
            refcount_ = nullptr;
            destroy_handle();
        }
    }

    void destroy_handle() noexcept {
        if (handle_) { handle_.destroy(); }
    }
};




namespace detail {


template<typename CoroT>
class GeneratorPromise {
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

    [[nodiscard]] auto extract_result() -> result_type {
        if      (is<result_type>       (result_)) { return move_out<result_type>(result_); }
        else if (is<std::exception_ptr>(result_)) { std::rethrow_exception(move_out<std::exception_ptr>(result_)); }
        else { std::terminate(); /* Invalid state. */ }
    }

private:
    struct no_result {};
    std::variant<no_result, result_type, std::exception_ptr> result_;
};


} // namespace detail




/*
NOTE: Not tested. Might work?
*/
template<typename T>
class Generator {
public:
    using result_type   = T;
    using value_type    = T;
    using promise_type  = detail::GeneratorPromise<Generator>;
    using handle_type   = std::coroutine_handle<promise_type>;
    using unique_handle = unique_coroutine_handle<promise_type>;

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
    unique_handle handle_;
};




namespace detail {


template<typename CoroT>
class JobPromise {
public:
    using coroutine_type = CoroT;
    using result_type    = coroutine_type::result_type;
    using promise_type   = JobPromise;
    using handle_type    = std::coroutine_handle<JobPromise>;
    using shared_handle  = shared_coroutine_handle<promise_type>;


    // Begin boilerplate.

    auto get_return_object() -> coroutine_type {
        return coroutine_type(handle_type::from_promise(*this));
    }

    void unhandled_exception() noexcept {
        result_value() = std::current_exception();
        became_ready();
    }

    auto initial_suspend() const noexcept -> std::suspend_never {
        return {};
    }

    auto final_suspend() const noexcept {
        struct FinalAwaiter {
            bool await_ready() const noexcept { return false; }
            void await_suspend(handle_type h) const noexcept {
                // Release ownership *after* suspension at the end.
                // If the Job was discarded, this will destroy the coroutine here.
                promise_type& p = h.promise();
                if (p.is_owning()) {
                    p.release_ownership();
                }
            }
            void await_resume() const noexcept {}
        };
        return FinalAwaiter{};
    }

    template<std::convertible_to<result_type> U>
    void return_value(U&& value) noexcept(noexcept(result_value() = FORWARD(value))) {
        assert(result_value().index() == 0);
        result_value() = FORWARD(value);
        became_ready();
    }

    // TODO: void specialization.
    // void return_void() noexcept { became_ready(); }

    // End boilerplate.


    // Whether the result/exception is ready to be retrieved by get_result().
    bool is_ready() const noexcept {
        const bool ready = ready_flag().load(std::memory_order_acquire);
        if (ready) { assert(result_value().index() != 0); }
        return ready;
    }

    void wait_for_result() const noexcept {
        ready_flag().wait(false, std::memory_order_acquire);
    }

    auto get_result() -> result_type& {
        if      (is<result_type>       (result_value())) { return get<result_type>(result_value()); }
        else if (is<std::exception_ptr>(result_value())) { std::rethrow_exception(get<std::exception_ptr>(result_value())); }
        else { std::terminate(); }
    }

    [[nodiscard]] auto extract_result() -> result_type {
        if      (is<result_type>       (result_value())) { return move_out<result_type>(result_value()); }
        else if (is<std::exception_ptr>(result_value())) { std::rethrow_exception(move_out<std::exception_ptr>(result_value())); }
        else { std::terminate(); /* Invalid state */ }
    }


    // Manual ownership handling.

    bool is_owning() const noexcept {
        return bool(handle_);
    }

    void give_ownership(shared_handle handle) noexcept {
        assert(!handle_);
        handle_ = MOVE(handle);
    }

    auto release_ownership() noexcept -> shared_handle {
        assert(handle_);
        return MOVE(handle_);
    }


    // Manual result handling.

    struct no_result {};
    using result_value_type =
        std::variant<no_result, result_type, std::exception_ptr>;

    // This can be emplaced externally. Does not signal readiness.
    template<std::convertible_to<result_value_type> U>
    void set_result_value(U&& value) {
        result_value() = FORWARD(value);
    }

    auto get_result_value() const -> const result_value_type& {
        return result_value();
    }

    // Take the result value away. Expects that readiness has *not* been signalled yet.
    [[nodiscard]] auto extract_result_value() -> result_value_type {
        assert(!is_ready());
        return move_out(result_value());
    }

private:
    struct ResultState {
        result_value_type value;
        std::atomic_bool  is_ready{ false }; // One-way transition flag.
    };

    // The promise will own the coroutine until final_suspend().
    // This is set through give_ownership() in the c-tor of the return object.
    shared_handle handle_ = handle_type(nullptr);
    ResultState   result_;

    auto ready_flag()         noexcept ->       std::atomic_bool& { return result_.is_ready; }
    auto ready_flag()   const noexcept -> const std::atomic_bool& { return result_.is_ready; }
    auto result_value()       noexcept ->       auto& { return result_.value; }
    auto result_value() const noexcept -> const auto& { return result_.value; }

    void became_ready() noexcept {
        ready_flag().store(true, std::memory_order_release);
        ready_flag().notify_all();
    }
};




} // namespace detail




/*
A flavour of the eager task that preserves it's lifetime as long as the
task is running.

Think of this as a future/promise pair augmentend with the actual
task attached to the promise.

The Job is a bridge from the coroutine world to the normal execution,
and is also a barrier that stops the coroutine-plague from propagating
through your whole codebase (cough-cough, lazy tasks...).

It's more heavyweight than a lazy task, so adjust your usage accordingly.
*/
template<typename T>
class Job : private MoveOnly<Job<T>> {
public:
    using result_type   = T;
    using value_type    = T;
    using promise_type  = detail::JobPromise<Job>;
    using handle_type   = std::coroutine_handle<promise_type>;
    using shared_handle = shared_coroutine_handle<promise_type>;

    // The Job will eagerly start on the current thread, then
    // an explicit rescheduling can be done by the coroutine.
    explicit Job(handle_type handle) : handle_{ handle } {
        handle_.get().promise().give_ownership(handle_);
    }

    // Is the task completed. Synchronized by an atomic.
    // If `true`, retrieving the result will not block.
    bool is_ready() const noexcept {
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
        -> result_type&
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
        -> awaiter<result_type&> auto
    {
        struct Awaiter {
            const Job& self;
            bool await_ready() const noexcept { return self.is_ready(); }
            bool await_suspend(std::coroutine_handle<>) const noexcept { return !self.is_ready(); }
            auto await_resume() const -> result_type& { return self.get_result(); }
        };
        return Awaiter{ *this };
    }

    auto operator co_await() && noexcept
        -> awaiter<result_type> auto
    {
        struct Awaiter {
            Job& self;
            bool await_ready() const noexcept { return self.is_ready(); }
            bool await_suspend(std::coroutine_handle<>) const noexcept { return !self.is_ready(); }
            auto await_resume() const -> result_type { return MOVE(self.get_result()); }
        };
        return Awaiter{ *this };
    }

private:
    shared_handle handle_;
    template<typename> friend class SharedJob;
};




/*
This is not a coroutine itself, but is a shared awaitable handle to a JobPromise.
*/
template<typename T>
class SharedJob : private Copyable<SharedJob<T>> {
public:
    using result_type = Job<T>::result_type;
    using value_type  = Job<T>::value_type;

    SharedJob(Job<T>&& job) : handle_{ MOVE(job.handle_) } {}

    // Is the task completed. Synchronized by an atomic.
    // If `true`, retrieving the result will not block.
    bool is_ready() const noexcept {
        return handle_ && handle_.get().promise().is_ready();
    }

    // Block until the job has finished.
    void wait_until_ready() const noexcept {
        assert(handle_);
        typename Job<T>::promise_type& p = handle_.get().promise();
        p.wait_for_result();
    }

    auto get_result() const
        -> const result_type&
    {
        assert(handle_);
        typename Job<T>::promise_type& p = handle_.get().promise();
        p.wait_for_result();
        return p.get_result();
    }

    auto operator co_await() const noexcept
        -> awaiter<const result_type&> auto
    {
        struct Awaiter {
            const SharedJob& self;
            bool await_ready() const noexcept { return self.is_ready(); }
            bool await_suspend(std::coroutine_handle<>) const noexcept { return !self.is_ready(); }
            auto await_resume() const -> const result_type& { return self.get_result(); }
        };
        return Awaiter{ *this };
    }


private:
    Job<T>::shared_handle handle_;
};




// Suspend current coroutine and resume it on the specified executor.
template<typename ExecutorT>
auto reschedule_to(ExecutorT& executor)
    -> awaiter auto
{
    struct Awaiter {
        ExecutorT& executor;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) {
            auto resumer = [h] { h.resume(); };

            using result_type =
                decltype([&]() -> decltype(auto) { return executor.emplace(MOVE(resumer)); }());

            if constexpr (std::is_void_v<result_type>) {
                executor.emplace(MOVE(resumer));
            } else {
                discard(executor.emplace(MOVE(resumer)));
            }
        }
        void await_resume() const noexcept {}
    };
    return Awaiter{ executor };
}




template<typename T>
struct readyable_traits {
    static bool is_ready(const T& readyable) { return readyable.is_ready(); }
};


template<typename T>
concept readyable = requires(T r) {
    { readyable_traits<T>::is_ready(r) } -> std::convertible_to<bool>;
};


// Suspend if the readyable is not ready.
template<readyable T>
auto when_ready(T&& readyable)
    -> awaiter<void> auto
{
    using rt = readyable_traits<std::remove_cvref_t<T>>;
    struct Awaiter {
        T readyable;
        bool await_ready() const noexcept { return rt::is_ready(readyable); }
        bool await_suspend(std::coroutine_handle<>) const noexcept { return !rt::is_ready(readyable); }
        void await_resume() const noexcept {}
    };
    return Awaiter{ FORWARD(readyable) };
}


// Suspend if the readyable is not ready.
//
// Resume with `false` if it still not ready after resumption.
// To keep suspending until the thing becomes ready:
//
//   while (!co_await if_not_ready(thing));
//
// The suspended coroutine handle can be retrieved by passing a second out parameter:
//
//   std::coroutine_handle<> suspended_self;
//   while (!co_await if_not_ready(thing, &suspended_self));
//
template<readyable T, typename PromiseT = void>
auto if_not_ready(T&& readyable, std::coroutine_handle<PromiseT>* out_suspended = nullptr)
    -> awaiter<bool> auto
{
    using rt = readyable_traits<std::remove_cvref_t<T>>;
    struct Awaiter {
        T                        readyable;
        std::coroutine_handle<>* out_suspended;
        bool await_ready() const noexcept { return rt::is_ready(readyable); }
        bool await_suspend(std::coroutine_handle<> h) const noexcept {
            const bool suspend = !rt::is_ready(readyable);
            if (suspend && out_suspended) {
                *out_suspended = h;
            }
            return suspend;
        }
        bool await_resume() const noexcept { return rt::is_ready(readyable); }
    };
    return Awaiter{ readyable, out_suspended };
}


} // namespace josh
