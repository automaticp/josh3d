#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <optional>
#include <utility>


template<typename T>
class ThreadsafeQueue {
private:
    std::queue<T> que_;
    mutable std::mutex mutex_;
    std::condition_variable_any push_cv_;

public:
    ThreadsafeQueue() = default;
    ThreadsafeQueue(const ThreadsafeQueue& other);
    ThreadsafeQueue(ThreadsafeQueue&& other) noexcept;
    ThreadsafeQueue& operator=(const ThreadsafeQueue& other) = delete;
    ThreadsafeQueue& operator=(ThreadsafeQueue&& other) = delete;

    void push(auto&& value);
    void emplace(auto&&... args);

    // Pop value and return.
    // If the queue is empty return nullopt.
    std::optional<T> try_pop();

    // Pop value and return.
    // If the queue is empty wait until a value is pushed.
    T wait_and_pop();

    // Pop value and return.
    // If the queue is empty wait until a value is pushed or a stop is requested.
    // On stop request return nullopt.
    std::optional<T> wait_and_pop(std::stop_token stoken);

    // Check if the queue is empty.
    // Do note that when this function returns the lock on the queue is released
    // and the queue can change it's state before the next call.
    // The result of this call is a suggestion at best. Prefer *_pop() functions instead.
    bool empty() const noexcept;

    // Invoke some callable while the underlying queue is locked.
    // Keep in mind that ANY calls to ThreadsafeQueue member functions
    // within the body of the callable is a GUARANTEED Deadlock.
    template<std::invocable CallableT>
    decltype(auto) lock_and(CallableT&& callable);

    template<std::invocable CallableT>
    decltype(auto) lock_and(CallableT&& callable) const;

    // Overload for callables that accept (const) std::queue<T>& as the argument.
    // You can poke around in the underlying queue, if you want.
    template<std::invocable<std::queue<T>&> CallableT>
    decltype(auto) lock_and(CallableT&& callable);

    template<std::invocable<const std::queue<T>&> CallableT>
    decltype(auto) lock_and(CallableT&& callable) const;
};


template<typename T>
ThreadsafeQueue<T>::ThreadsafeQueue(const ThreadsafeQueue& other) {
    std::lock_guard lk{ other.mutex_ };
    que_ = other.que_;
}


template<typename T>
ThreadsafeQueue<T>::ThreadsafeQueue(ThreadsafeQueue&& other) noexcept {
    std::lock_guard lk{ other.mutex_ };
    que_ = std::move(other.que_);
}


template<typename T>
void ThreadsafeQueue<T>::push(auto&& value) {
    std::lock_guard lk{ mutex_ };
    que_.push(std::forward<decltype(value)>(value));
    push_cv_.notify_one();
}


template<typename T>
template<typename ...Args>
void ThreadsafeQueue<T>::emplace(Args&&... args) {
    std::lock_guard lk{ mutex_ };
    que_.emplace(std::forward<Args>(args)...);
    push_cv_.notify_one();
}


template<typename T>
T ThreadsafeQueue<T>::wait_and_pop() {
    std::unique_lock lk{ mutex_ };
    push_cv_.wait(lk, [this]{ return !que_.empty(); });
    T value = std::move(que_.front());
    que_.pop();
    return value;
}


template<typename T>
std::optional<T> ThreadsafeQueue<T>::wait_and_pop(std::stop_token stoken) {
    std::unique_lock lk{ mutex_ };
    bool predicate_result =
        push_cv_.wait(lk, stoken, [this] { return !que_.empty(); });

    if (!predicate_result) {
        return std::nullopt;
    }

    T value = std::move(que_.front());
    que_.pop();
    return std::move(value);
}


template<typename T>
std::optional<T> ThreadsafeQueue<T>::try_pop() {
    std::lock_guard lk{ mutex_ };
    if (!que_.empty()) {
        auto value = std::move(que_.front());
        que_.pop();
        return std::move(value);
    } else {
        return std::nullopt;
    }
}


template<typename T>
bool ThreadsafeQueue<T>::empty() const noexcept {
    std::lock_guard lk{ mutex_ };
    return que_.empty();
}


template<typename T>
template<std::invocable CallableT>
decltype(auto) ThreadsafeQueue<T>::lock_and(CallableT&& callable) {
    std::lock_guard lk{ mutex_ };
    return callable();
}


template<typename T>
template<std::invocable CallableT>
decltype(auto) ThreadsafeQueue<T>::lock_and(CallableT&& callable) const {
    std::lock_guard lk{ mutex_ };
    return callable();
}


template<typename T>
template<std::invocable<std::queue<T>&> CallableT>
decltype(auto) ThreadsafeQueue<T>::lock_and(CallableT&& callable) {
    std::lock_guard lk{ mutex_ };
    return callable(que_);
}


template<typename T>
template<std::invocable<const std::queue<T>&> CallableT>
decltype(auto) ThreadsafeQueue<T>::lock_and(CallableT&& callable) const {
    std::lock_guard lk{ mutex_ };
    return callable(que_);
}

