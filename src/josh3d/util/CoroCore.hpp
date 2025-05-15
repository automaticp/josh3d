#pragma once
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include "ContainerUtils.hpp"
#include "Ranges.hpp"
#include <boost/scope/scope_exit.hpp>
#include <atomic>
#include <cassert>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <ranges>
#include <type_traits>
#include <utility>


namespace josh {


template<typename T>
concept await_suspend_return_type =
    any_of<T, void, bool> ||
    specialization_of<T, std::coroutine_handle>;


template<typename T, typename ResultT = void, typename PromiseT = void>
concept awaiter = requires(T awaiter, std::coroutine_handle<PromiseT> coroutine) {
    { awaiter.await_ready()            } -> std::convertible_to<bool>;
    { awaiter.await_suspend(coroutine) } -> await_suspend_return_type;
    { awaiter.await_resume()           } -> std::same_as<ResultT>;
};


template<typename T, typename ResultT = void, typename PromiseT = void>
concept awaitable =
    awaiter<T, ResultT, PromiseT> ||
    requires(T awaitable) { { awaitable.operator co_await() } -> awaiter<ResultT, PromiseT>; } ||
    requires(T awaitable) { { operator co_await(awaitable)  } -> awaiter<ResultT, PromiseT>; };


/*
FIXME: emplace() is not the greatest default name to standardize as
the executor discriminator. We only have it here because ThreadPool
originally had emplace in its interface.

TODO: The emplace()-like function should be indirected through a trait.
*/
template<typename T>
concept executor = requires(T executor) {
    { executor.emplace([](){}) };
};


/*
Suspend the current coroutine and resume it on the specified executor.
*/
template<executor E>
[[nodiscard]]
auto reschedule_to(E& executor)
    -> awaiter<void> auto
{
    struct Awaiter {
        E& executor;
        auto await_ready() const noexcept -> bool { return false; }
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
struct readyable_traits;


namespace detail {
template<typename T>
concept has_member_is_ready = requires(T v) {
    { v.is_ready() } -> std::convertible_to<bool>;
};
template<typename T>
concept has_adl_is_ready = requires(T v) {
    { is_ready(v) } -> std::convertible_to<bool>;
};
template<typename T>
concept specializes_readyable_traits = requires(T v) {
    { readyable_traits<T>::is_ready(v) } -> std::convertible_to<bool>;
};
} // namespace detail


/*
Anything that the "ready" status can be queried on.

NOTE: The name is awful but it will probably stay.
*/
template<typename T>
concept readyable =
    detail::has_member_is_ready<T> or
    detail::has_adl_is_ready<T> or
    detail::specializes_readyable_traits<T>;


namespace cpo {
/*
Niebloid cpo thing for `is_ready(r)`.
*/
constexpr struct is_ready_fn {
    template<readyable R_>
    auto operator()(R_&& readyable) const
        -> bool
    {
        using R = std::decay_t<R_>;
        if constexpr (detail::has_member_is_ready<R>)
            return bool(readyable.is_ready());
        else if constexpr (detail::has_adl_is_ready<R>)
            return bool(is_ready(readyable));
        else if constexpr (detail::specializes_readyable_traits<R>)
            return bool(readyable_traits<R>::is_ready(readyable));
        else static_assert(false_v<R>);
    }
} is_ready;
} // namespace cpo


/*
Adapt an arbitrary predicate as a readyable on the fly.
*/
template<of_signature<bool()> F>
constexpr auto as_readyable(F&& f) noexcept
    -> readyable auto
{
    struct ReadyableAdaptor {
        F func;
        constexpr auto is_ready() const { return func(); }
    };
    return ReadyableAdaptor(FORWARD(f));
}


/*
Suspend if the readyable is not ready.

Resume with `false` if it still not ready after resumption.
To keep suspending until the thing becomes ready:

  while (!co_await if_not_ready(thing));

The suspended coroutine handle can be retrieved by passing a second out parameter:

  std::coroutine_handle<> suspended_self;
  while (!co_await if_not_ready(thing, &suspended_self));

*/
template<readyable T, typename PromiseT = void>
[[nodiscard]]
auto if_not_ready(T&& readyable, std::coroutine_handle<PromiseT>* out_suspended = nullptr)
    -> awaiter<bool> auto
{
    using cpo::is_ready;
    struct Awaiter {
        T                                readyable;
        std::coroutine_handle<PromiseT>* out_suspended;
        auto await_ready() const noexcept -> bool { return is_ready(readyable); }
        auto await_suspend(std::coroutine_handle<PromiseT> h) const noexcept
            -> bool
        {
            const bool suspend = not is_ready(readyable);
            if (suspend && out_suspended) {
                *out_suspended = h;
            }
            return suspend;
        }
        auto await_resume() const noexcept -> bool { return is_ready(readyable); }
    };
    return Awaiter{ readyable, out_suspended };
}


/*
Suspends the current coroutine to get its address, then resumes it.
Can be useful to get a unique identifier for each coroutine.
*/
[[nodiscard]]
inline auto peek_coroutine_address()
    -> awaiter<void*> auto
{
    struct Awaiter {
        void* result;
        auto await_ready() const noexcept -> bool { return false; }
        auto await_suspend(std::coroutine_handle<> h) noexcept
            -> bool
        {
            result = h.address();
            // Then just pray that the compiler is smart enough to not actually suspend.
            return false;
        }
        auto await_resume() const noexcept -> void* { return result; }
    };
    return Awaiter{};
}


namespace detail {

// The implementation is surprisingly elaborate due to the need
// to "weave" state and control flow through the already-complicated
// coroutines API. This is the true inversion of control horror.

struct WhenAllState {
    std::coroutine_handle<> parent_coroutine;
    std::atomic<size_t>     num_remaining;
    std::exception_ptr      exception = nullptr;
    std::atomic_flag        exception_is_set = false;
};

struct CompletionNotifier {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    // NOTE: Not owning the coroutine_handle.
    // It will deal with itself from within the final suspend.
    handle_type handle;

    struct promise_type {
        WhenAllState* state = nullptr;

        auto get_return_object() -> CompletionNotifier { return { handle_type::from_promise(*this) }; }

        // NOTE: What we do here is pretty dumb, we record only the first exception.
        // Everything else that failed after is swallowed.
        //
        // TODO: There must be a better way to handle this, but multiplexing exceptions
        // is a major PITA already. And I have no obvious ideas right now.
        //
        // NOTE: This is done here instead of doing it in the actual completion coroutine
        // body because that trips up ASan in some wierd way. Or maybe it was a real bug?
        void unhandled_exception() noexcept {
            assert(state);
            const bool already_set =
                state->exception_is_set.test_and_set(std::memory_order_acq_rel);
            if (not already_set) {
                assert(not state->exception);
                state->exception = std::current_exception();
            }
        }

        void return_void() const noexcept {}

        // NOTE: Will suspend initially and be manually started after
        // setting the state pointer. This is somewhat weird, ohwell.
        auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

        // Decrement counter, destroy self, transfer control to parent coroutine if last.
        auto final_suspend() const noexcept
            -> awaiter<void> auto
        {
            struct FinalAwaiter {
                WhenAllState* state;
                auto await_ready() const noexcept -> bool { return false; }
                auto await_suspend(std::coroutine_handle<> self) const noexcept
                    -> std::coroutine_handle<>
                {
                    assert(state);
                    assert(state->parent_coroutine);

                    const auto destroy_self = boost::scope::scope_exit([&]{ self.destroy(); });

                    if (state->num_remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                        return state->parent_coroutine;
                    } else {
                        return std::noop_coroutine();
                    }
                }
                // NOTE: Doing nothing on unhandled exception. Our job is only to wait,
                // not propagate the exception. The calling side will later inspect the
                // result to find out the exact state.
                void await_resume() const noexcept {}
            };
            return FinalAwaiter(state);
        }
    };
};
} // namespace detail


/*
Suspend until all awaitables in a range have become ready.

Execution will resume on the context chosen by the last awaitable.
Usually, that will be the same thread a coroutine finished on.
*/
template<std::ranges::sized_range R>
    requires std::ranges::borrowed_range<R>
[[nodiscard]]
auto until_all_ready(R&& awaitables)
    -> awaitable<void> auto
{
    using detail::WhenAllState, detail::CompletionNotifier;
    struct WhenAllReadyAwaitable {
        // NOTE: The state must be stored in the parent coroutine.
        // Therefore it is stuffed into the awaitable itself.
        WhenAllState state;
        R            awaitables;

        auto operator co_await()
            -> awaiter<void> auto
        {
            struct Awaiter {
                WhenAllReadyAwaitable& self;
                // NOTE: Need to suspend to set the parent coroutine first.
                // Can only resume early if the range is empty.
                auto await_ready() noexcept
                    -> bool
                {
                    return std::ranges::size(self.awaitables) == 0;
                }
                auto await_suspend(std::coroutine_handle<> parent_coroutine) noexcept
                    -> std::coroutine_handle<>
                {
                    self.state.parent_coroutine = parent_coroutine;

                    auto attach_continuation = [](auto& awaitable)
                        -> CompletionNotifier
                    {
                        co_await awaitable;
                    };

                    // Loop over all but last. The last will have control transferred to it instead.
                    const size_t last = std::ranges::size(self.awaitables) - 1;
                    for (auto [i, awaitable] : enumerate(self.awaitables)) {
                        CompletionNotifier continuation = attach_continuation(awaitable);
                        // We have to sneak-in the state pointer into the promise.
                        // This is why the CompletionNotifier suspends initially.
                        continuation.handle.promise().state = &self.state;
                        if (i != last) {
                            continuation.handle.resume();
                        } else /* last */ {
                            return continuation.handle;
                        }
                    }
                    safe_unreachable();
                }
                void await_resume() const noexcept {}
            };
            return Awaiter{ *this };
        }
    };

    return WhenAllReadyAwaitable{
        .state = {
            .parent_coroutine = {}, // NOTE: Will be set upon suspension.
            .num_remaining    = std::ranges::size(awaitables),
        },
        .awaitables = FORWARD(awaitables)
    };
}


/*
Suspend until all awaitables in a range have become ready.

Will propagate the first exception thrown by an awaitable, if any,
but only *after* all awaitables have become ready.

Execution will resume on the context chosen by the last awaitable.
Usually, that will be the same thread a coroutine finished on.
*/
template<std::ranges::sized_range R>
    requires std::ranges::borrowed_range<R>
[[nodiscard]]
auto until_all_succeed(R&& awaitables)
    -> awaitable<void> auto
{
    using detail::WhenAllState, detail::CompletionNotifier;
    struct WhenAllSucceedAwaitable {
        WhenAllState       state;
        R                  awaitables;

        auto operator co_await()
            -> awaiter<void> auto
        {
            struct Awaiter {
                WhenAllSucceedAwaitable& self;
                auto await_ready() noexcept
                    -> bool
                {
                    return std::ranges::size(self.awaitables) == 0;
                }
                auto await_suspend(std::coroutine_handle<> parent_coroutine) noexcept
                    -> std::coroutine_handle<>
                {
                    self.state.parent_coroutine = parent_coroutine;

                    auto attach_continuation = [](auto& awaitable)
                        -> CompletionNotifier
                    {
                        co_await awaitable;
                    };

                    const size_t last = std::ranges::size(self.awaitables) - 1;
                    for (auto [i, awaitable] : enumerate(self.awaitables)) {
                        CompletionNotifier continuation = attach_continuation(awaitable);
                        // We have to sneak-in the state pointer into the promise.
                        // This is why the CompletionNotifier suspends initially.
                        continuation.handle.promise().state = &self.state;
                        if (i != last) {
                            continuation.handle.resume();
                        } else /* last */ {
                            return continuation.handle;
                        }
                    }
                    safe_unreachable();
                }
                void await_resume() const {
                    // NOTE: This is the main difference between until_all_ready() and until_all_succeed().
                    if (self.state.exception) std::rethrow_exception(self.state.exception);
                }
            };
            return Awaiter{ *this };
        }
    };

    return WhenAllSucceedAwaitable{
        .state = {
            .parent_coroutine = {}, // NOTE: Will be set upon suspension.
            .num_remaining    = std::ranges::size(awaitables),
        },
        .awaitables = FORWARD(awaitables)
    };

}




template<typename PromiseT = void>
class unique_coroutine_handle {
public:
    unique_coroutine_handle(std::coroutine_handle<PromiseT> handle) noexcept
        : handle_{ handle }
    {}

    auto operator->() const noexcept -> std::coroutine_handle<PromiseT>* { return &handle_; }
    auto operator*()  const noexcept -> std::coroutine_handle<PromiseT>& { return handle_;  }
    auto get()        const noexcept -> std::coroutine_handle<PromiseT>  { return handle_;  }

    explicit operator bool() const noexcept { return bool(handle_); }
    bool valid()             const noexcept { return bool(handle_); }

    operator unique_coroutine_handle<>() && noexcept { return { std::exchange(handle_, nullptr) }; }


    unique_coroutine_handle(const unique_coroutine_handle&)           = delete;
    unique_coroutine_handle operator=(const unique_coroutine_handle&) = delete;

    unique_coroutine_handle(unique_coroutine_handle&& other) noexcept
        : handle_{ std::exchange(other.handle_, nullptr) }
    {}

    unique_coroutine_handle& operator=(unique_coroutine_handle&& other) noexcept {
        if (handle_) handle_.destroy();
        handle_ = std::exchange(other.handle_, nullptr);
        return *this;
    }

    ~unique_coroutine_handle() noexcept { if (handle_) handle_.destroy(); }

private:
    std::coroutine_handle<PromiseT> handle_;
    template<typename U> friend class shared_coroutine_handle;
};




template<typename PromiseT = void>
class shared_coroutine_handle {
public:
    shared_coroutine_handle(std::coroutine_handle<PromiseT> handle) noexcept
        : handle_  { handle               }
        , refcount_{ new refcount_type(1) }
    {}

    auto operator->() const noexcept -> const std::coroutine_handle<PromiseT>* { return &handle_; }
    auto operator*()  const noexcept -> const std::coroutine_handle<PromiseT>& { return handle_;  }
    auto get()        const noexcept ->       std::coroutine_handle<PromiseT>  { return handle_;  }

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
    shared_coroutine_handle(unique_coroutine_handle<PromiseT>&& other) noexcept
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
    auto operator=(unique_coroutine_handle<PromiseT>&& other) noexcept
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

    std::coroutine_handle<PromiseT> handle_;
    refcount_type*           refcount_;

    struct PrivateKey {};

    template<typename U> friend class shared_coroutine_handle;

    shared_coroutine_handle(std::coroutine_handle<PromiseT> handle, refcount_type* refcount, PrivateKey)
        : handle_  { handle   }
        , refcount_{ refcount }
    {}

    shared_coroutine_handle(std::coroutine_handle<PromiseT> handle, PrivateKey)
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


} // namespace josh
