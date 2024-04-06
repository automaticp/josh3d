#pragma once
#include "RuntimeError.hpp"
#include <atomic>
#include <exception>
#include <type_traits>
#include <variant>
#include <memory>
#include <utility>
#include <cassert>



namespace josh {


template<typename T>
class Future;

template<typename T>
class Promise;


template<typename T>
auto make_future_promise_pair() -> std::pair<Future<T>, Promise<T>>;


template<typename T>
auto get_result(Future<T> future) -> T;


template<typename T>
void set_result(Promise<T> promise, T result);


template<typename T>
void set_exception(Promise<T> promise, std::exception_ptr exception);



namespace error {

// Not a logic_error, valid outcome?
class BrokenPromise : public RuntimeError {
public:
    static constexpr auto prefix = "Broken Promise";
    BrokenPromise() : RuntimeError{ prefix, "" } {}
};

} // namespace error



namespace detail {

template<typename T>
struct FPState {
    using storage_type = std::variant<std::monostate, T, std::exception_ptr>;
    storage_type     value_or_exception{};
    std::atomic_flag ready = ATOMIC_FLAG_INIT;
    // Imagine not guaranteeing clear state before C++20...
};

} // namespace detail




template<typename T>
class Promise {
public:
    Promise() = delete;
    friend auto make_future_promise_pair<T>() -> std::pair<Future<T>, Promise<T>>;

    bool is_moved_from() const noexcept; // Is this needed?
    friend void set_result<T>(Promise<T> promise, T result);
    friend void set_exception<T>(Promise<T> promise, std::exception_ptr exception);

    Promise(const Promise&)            = delete;
    Promise& operator=(const Promise&) = delete;
    Promise(Promise&&)                 = default;
    Promise& operator=(Promise&&)      = default;

    ~Promise() noexcept;
private:
    Promise(std::shared_ptr<detail::FPState<T>>&& state) noexcept : state_{ std::move(state) } {}
    std::shared_ptr<detail::FPState<T>> state_;
};



template<typename T>
class Future {
public:
    Future() = delete;
    friend auto make_future_promise_pair<T>() -> std::pair<Future<T>, Promise<T>>;

    bool is_moved_from() const noexcept; // Is this needed?
    bool is_available() const noexcept;
    void wait_for_result() const noexcept;
    friend auto get_result<T>(Future<T> future) -> T;

    Future(const Future&)            = delete;
    Future& operator=(const Future&) = delete;
    Future(Future&&)                 = default;
    Future& operator=(Future&&)      = default;

private:
    Future(std::shared_ptr<detail::FPState<T>>&& state) noexcept : state_{ std::move(state) } {}
    std::shared_ptr<detail::FPState<T>> state_;
};




template<typename T>
auto make_future_promise_pair()
    -> std::pair<Future<T>, Promise<T>>
{
    auto ptr_promise = std::make_shared<detail::FPState<T>>();
    auto ptr_future  = ptr_promise;
    return { Future<T>(std::move(ptr_future)), Promise<T>(std::move(ptr_promise)) };
}




template<typename T>
bool Promise<T>::is_moved_from() const noexcept {
    return !bool(state_);
}

template<typename T>
void set_result(Promise<T> promise, T result) {
    promise.state_->value_or_exception = std::move(result);

#ifndef NDEBUG
    bool was_set_before =
        promise.state_->ready.test_and_set(std::memory_order_acq_rel);
    // Could be just memory_order_release, if it wasn't for this assert.
    assert(!was_set_before);
#else
    promise.state_->ready.test_and_set(std::memory_order_release);
#endif

    promise.state_->ready.notify_one();

    // Will destroy the value here if promise is the only owner,
    // that is, if the Future has been discarded.
}

template<typename T>
void set_exception(Promise<T> promise, std::exception_ptr exception) { // NOLINT(performance-unnecessary-value-param)
    promise.state_->value_or_exception = std::move(exception);

#ifndef NDEBUG
    bool was_set_before =
        promise.state_->ready.test_and_set(std::memory_order_acq_rel);
    // Could be just memory_order_release, if it wasn't for this assert.
    assert(!was_set_before);
#else
    promise.state_->ready.test_and_set(std::memory_order_release);
#endif

    promise.state_->ready.notify_one();
}

template<typename T>
Promise<T>::~Promise() noexcept {
    // Broooken proooomiseees...
    if (!is_moved_from() &&
        !state_->ready.test(std::memory_order_acquire))
    {
        state_->value_or_exception = std::make_exception_ptr(error::BrokenPromise());
        state_->ready.test_and_set(std::memory_order_release);
        state_->ready.notify_one();
    }
}




template<typename T>
bool Future<T>::is_moved_from() const noexcept {
    return !bool(state_);
}

template<typename T>
bool Future<T>::is_available() const noexcept {
    // TODO: Should we check is_moved_from()?
    return state_->ready.test(std::memory_order_acquire);
}

template<typename T>
void Future<T>::wait_for_result() const noexcept {
    state_->ready.wait(false, std::memory_order_acquire);
}

template<typename T>
auto get_result(Future<T> future)
    -> T
{
    future.wait_for_result();

    struct Visitor {
        T operator()(T&& value) const {
            return std::move(value);
        }
        [[noreturn]] T operator()(std::exception_ptr&& exception) const {
            std::rethrow_exception(std::move(exception));
        }
        [[noreturn]] T operator()(std::monostate) const {
            assert(false);
        }
    };

    return std::visit(Visitor{}, std::move(future.state_->value_or_exception));
}



} // namespace josh
