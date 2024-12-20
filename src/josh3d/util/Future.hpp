#pragma once
#include <atomic>
#include <concepts>
#include <exception>
#include <stdexcept>
#include <variant>
#include <memory>
#include <utility>
#include <cassert>



namespace josh {


template<typename T>
concept not_void = !std::same_as<T, void>;


template<typename T>
class Future;

template<typename T>
class Promise;


template<typename T>
auto make_future_promise_pair() -> std::pair<Future<T>, Promise<T>>;


template<typename T>
auto get_result(Future<T> future) -> T;


template<not_void T>
void set_result(Promise<T> promise, T result);
void set_result(Promise<void> promise);


template<typename T>
void set_exception(Promise<T> promise, std::exception_ptr exception);




class broken_promise : public std::logic_error {
public:
    using std::logic_error::logic_error;
};




namespace detail {

template<typename T>
struct FPState {
    using storage_type = std::variant<std::monostate, T, std::exception_ptr>;
    storage_type     value_or_exception{};
    std::atomic_flag ready = ATOMIC_FLAG_INIT;
    // Imagine not guaranteeing clear state before C++20...
};

template<>
struct FPState<void> {
    using storage_type = std::exception_ptr;
    storage_type     value_or_exception = nullptr;
    std::atomic_flag ready = ATOMIC_FLAG_INIT;
};

} // namespace detail




template<typename T>
class Promise {
public:
    Promise() = delete;
    friend auto make_future_promise_pair<T>() -> std::pair<Future<T>, Promise<T>>;

    bool is_moved_from() const noexcept; // Is this needed?
    friend void set_result(Promise<void> promise);
    template<not_void U> friend void set_result(Promise<U> promise, U result); // Template so that there's no complaining about void.
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

template<not_void T>
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

inline void set_result(Promise<void> promise) {
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
        state_->value_or_exception = std::make_exception_ptr(broken_promise("broken_promise"));
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
            std::terminate();
        }
    };

    return std::visit(Visitor{}, std::move(future.state_->value_or_exception));
}


template<>
inline void get_result<void>(Future<void> future) {

    future.wait_for_result();

    if (future.state_->value_or_exception) {
        std::rethrow_exception(std::move(future.state_->value_or_exception));
    }
}



} // namespace josh
