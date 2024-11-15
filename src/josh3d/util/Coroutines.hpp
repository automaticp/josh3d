#pragma once
#include <concepts>
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>
#include <variant>


/*
NOTE: Untested and maybe incomplete.
*/
namespace josh {


template<typename T = void>
class unique_coroutine_handle {
public:
    explicit unique_coroutine_handle(std::coroutine_handle<T> handle) noexcept
        : handle_{ handle }
    {}

    auto get() const noexcept -> std::coroutine_handle<T> { return handle_; }

    operator unique_coroutine_handle<>() && noexcept { return { std::exchange(handle_, nullptr) }; }

    explicit operator bool() const noexcept { return bool(handle_); }

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
};


template<typename T>
class GeneratorPromise;


template<typename T>
class Generator {
public:
    using result_type   = T;
    using promise_type  = GeneratorPromise<Generator>;
    using handle_type   = std::coroutine_handle<promise_type>;
    using unique_handle = unique_coroutine_handle<promise_type>;

    Generator(handle_type handle) : handle_{ handle } {}

    auto operator()() -> std::optional<result_type>;

private:
    unique_handle handle_;
};


template<typename GenCoroT>
class GeneratorPromise {
public:
    using coroutine_type = GenCoroT;
    using handle_type    = coroutine_type::handle_type;
    using result_type    = coroutine_type::result_type;

    // Begin boilerplate.

    auto get_return_object() -> coroutine_type { return coroutine_type(handle_type::from_promise(*this)); }

    auto initial_suspend() const noexcept -> std::suspend_always { return {}; }
    auto final_suspend()   const noexcept -> std::suspend_always { return {}; }

    void unhandled_exception() noexcept { result_ = std::current_exception(); }

    template<std::convertible_to<GenCoroT> U>
    auto yield_value(U&& value) -> std::suspend_always { result_ = std::forward<U>(value); return {}; }

    void return_void() const noexcept {}

    // End boilerplate.


    auto has_any_result() const noexcept -> bool {
        const bool is_empty = (result_.index() == 0 && !get<0>(result_));
        return !is_empty;
    }

    [[nodiscard]] auto extract_result() const -> result_type {
        if (auto* exception = get_if<std::exception_ptr>(result_)) {
            if (*exception) {
                std::rethrow_exception(*exception);
            } else {
                std::terminate(); // assert(false).
            }
        }

        auto& value = get<result_type>(result_);
        auto returned_value = std::move(value);
        result_ = {}; // Reset to null exception_ptr to signal that we extracted the value.
        return std::move(returned_value);
    }

private:
    // Default-constructed exception_ptr is used for "empty" promise state.
    std::variant<std::exception_ptr, result_type> result_;
};


template<typename T>
inline auto Generator<T>::operator()()
    -> std::optional<result_type>
{
    handle_type   h = handle_.get();
    promise_type& p = h.promise();

    if (!h.done()) {
        h.resume(); // UB if h.done(), so guard.
        // Check again, because we might have `co_return`ed after last resume().
        if (!h.done()) {
            return { std::move(p.extract_result()) };
        }
    }

    return std::nullopt;
}


} // namespace josh
