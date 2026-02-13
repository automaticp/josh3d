#pragma once
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include "Scalars.hpp"
#include "Semantics.hpp"
#include <atomic>
#include <utility>


namespace josh {
namespace detail {

template<typename T>
struct UsageValue
{
    using value_type = T;

    explicit UsageValue() = default;
    UsageValue(value_type value) : _value{ MOVE(value) } {}

    // Value of the "used" item if has_usage() is true. T{} otherwise.
    auto value() const noexcept -> const T& { return _value; }

    UsageValue(const UsageValue& other)            = default;
    UsageValue& operator=(const UsageValue& other) = default;

    UsageValue(UsageValue&& other) noexcept
        : _value{ std::exchange(other._value, value_type{}) }
    {}

    UsageValue& operator=(UsageValue&& other) noexcept
    {
        _value = std::exchange(other._value, value_type{});
        return *this;
    }

    T _value{};
};

/*
EBO for Usage<void>.
*/
template<>
struct UsageValue<void>
{
    using value_type = void;
};


template<typename RefCountT>
struct UsageRC
{
    using refcount_type = RefCountT;

    explicit UsageRC() = default;

    UsageRC(refcount_type& refcount)
        : _refcount_ptr{ &refcount }
    {
        _increment_count();
    }

    // Does this own "usage" of a real item, or is this a "null" usage?
    bool has_usage() const noexcept { return bool(_refcount_ptr); }

    UsageRC(const UsageRC& other) noexcept
        : _refcount_ptr{ other._refcount_ptr }
    {
        if (_refcount_ptr) _increment_count();
    }

    auto operator=(const UsageRC& other) noexcept -> UsageRC&
    {
        if (this == &other) return *this; // To please clang-tidy. Not actually needed.
        if (_refcount_ptr) _decrement_count();
        _refcount_ptr = other._refcount_ptr;
        if (_refcount_ptr) _increment_count();
        return *this;
    }

    UsageRC(UsageRC&& other) noexcept
        : _refcount_ptr{ std::exchange(other._refcount_ptr, nullptr) }
    {}

    auto operator=(UsageRC&& other) noexcept -> UsageRC&
    {
        if (_refcount_ptr) _decrement_count();
        _refcount_ptr = std::exchange(other._refcount_ptr, nullptr);
        return *this;
    }

    ~UsageRC() noexcept
    {
        if (_refcount_ptr) _decrement_count();
    }

    refcount_type* _refcount_ptr = {};

private:
    void _increment_count() noexcept
    {
        _refcount_ptr->fetch_add(1, std::memory_order_relaxed);
    }

    void _decrement_count() noexcept
    {
        // Not acq_rel since we don't read the result.
        // This technically doesn't synchronize with anything and is just like relaxed.
        _refcount_ptr->fetch_sub(1, std::memory_order_release);
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
template<typename T, typename RefCount = std::atomic<size_t>>
struct [[nodiscard]] Usage
    : Copyable<Usage<T>>
    , detail::UsageValue<T>
    , detail::UsageRC<RefCount>
{
    Usage() = default;

    Usage(T value, RefCount& refcount)
        : detail::UsageValue<T>    { MOVE(value) }
        , detail::UsageRC<RefCount>{ refcount    }
    {}
};

template<typename RefCount>
struct [[nodiscard]] Usage<void, RefCount>
    : Copyable<Usage<void>>
    , detail::UsageValue<void>
    , detail::UsageRC<RefCount>
{
    Usage() = default;

    Usage(RefCount& refcount)
        : detail::UsageRC<RefCount>{ refcount }
    {}
};


} // namespace josh
