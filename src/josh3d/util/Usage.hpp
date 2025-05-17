#pragma once
#include "CategoryCasts.hpp"
#include "Semantics.hpp"
#include <atomic>
#include <utility>


namespace josh {
namespace detail {


template<typename T>
class UsageValue {
public:
    using value_type = T;

    explicit UsageValue() = default;
    UsageValue(value_type value) : value_{ MOVE(value) } {}

    // Value of the "used" item if has_usage() is true. T{} otherwise.
    auto value() const noexcept -> const T& { return value_; }

    UsageValue(const UsageValue& other)            = default;
    UsageValue& operator=(const UsageValue& other) = default;

    UsageValue(UsageValue&& other) noexcept
        : value_{ std::exchange(other.value_, value_type{}) }
    {}

    UsageValue& operator=(UsageValue&& other) noexcept {
        value_ = std::exchange(other.value_, value_type{});
        return *this;
    }

protected:
    T value_{};
};


// EBO for UsageToken<void>.
template<>
class UsageValue<void> {
public:
    using value_type = void;
};


template<typename RefCountT>
class UsageRC {
public:
    using refcount_type = RefCountT;

    explicit UsageRC() = default;

    UsageRC(refcount_type& refcount)
        : refcount_ptr_{ &refcount }
    {
        increment_count();
    }

    // Does this own "usage" of a real item, or is this a "null" usage?
    bool has_usage() const noexcept { return bool(refcount_ptr_); }

    UsageRC(const UsageRC& other) noexcept
        : refcount_ptr_{ other.refcount_ptr_ }
    {
        if (refcount_ptr_) increment_count();
    }

    UsageRC& operator=(const UsageRC& other) noexcept {
        if (this == &other) return *this; // To please clang-tidy. Not actually needed.
        if (refcount_ptr_) decrement_count();
        refcount_ptr_ = other.refcount_ptr_;
        if (refcount_ptr_) increment_count();
        return *this;
    }

    UsageRC(UsageRC&& other) noexcept
        : refcount_ptr_{ std::exchange(other.refcount_ptr_, nullptr) }
    {}

    UsageRC& operator=(UsageRC&& other) noexcept {
        if (refcount_ptr_) decrement_count();
        refcount_ptr_ = std::exchange(other.refcount_ptr_, nullptr);
        return *this;
    }

    ~UsageRC() noexcept {
        if (refcount_ptr_) decrement_count();
    }

protected:
    refcount_type* refcount_ptr_{};

private:
    void increment_count() noexcept {
        refcount_ptr_->fetch_add(1, std::memory_order_relaxed);
    }
    void decrement_count() noexcept {
        // Not acq_rel since we don't read the result.
        // This technically doesn't synchronize with anything and is just like relaxed.
        refcount_ptr_->fetch_sub(1, std::memory_order_release);
    }
};


} // namespace detail


/*
Shared ownership of a *thing*, in some ways similar to shared_ptr, except that
the lifetime of both T and the control block are not directly managed by the owners,
and are not destroyed with the last owner.

T is likely a pointer, identifier or a handle-like type that is small.
T must be default-constructible, although that shouldn't really be required.

This sounds like GC, but the disposal semantics are not defined here.
It is up to the system to decide on what to do with "unused" things.

NOTE: This is currently not fleshed out, the idea is to track transfer of usage
in more detail through a special control block type, not through a dumb atomic refcount,
with hooks for private->public transfer of ownership, "release hints", etc.
*/
template<typename T, typename RefCountT = std::atomic<size_t>>
class [[nodiscard]] Usage
    : public Copyable<Usage<T>>
    , public detail::UsageValue<T>
    , public detail::UsageRC<RefCountT>
{
public:
    explicit Usage() = default;

    Usage(T value, RefCountT& refcount)
        : detail::UsageValue<T>     { MOVE(value) }
        , detail::UsageRC<RefCountT>{ refcount    }
    {}

};


} // namespace josh
