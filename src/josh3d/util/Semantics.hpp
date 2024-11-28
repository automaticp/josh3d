#pragma once
#include <concepts>


namespace josh {


template<typename T>
concept immovable = !std::movable<T>;


template<typename T>
concept move_only = std::movable<T> && !std::copyable<T>;


template<typename T>
concept copyable = std::copyable<T>;


template<typename CRTP>
struct Immovable {
    constexpr Immovable() noexcept { static_assert(immovable<CRTP>); }
    constexpr Immovable(const Immovable&)            = delete;
    constexpr Immovable(Immovable&&)                 = delete;
    constexpr Immovable& operator=(const Immovable&) = delete;
    constexpr Immovable& operator=(Immovable&&)      = delete;
};


template<typename CRTP>
struct MoveOnly {
    constexpr MoveOnly() noexcept { static_assert(move_only<CRTP>); }
    constexpr MoveOnly(const MoveOnly&)            = delete;
    constexpr MoveOnly(MoveOnly&&)                 = default;
    constexpr MoveOnly& operator=(const MoveOnly&) = delete;
    constexpr MoveOnly& operator=(MoveOnly&&)      = default;
};


template<typename CRTP>
struct Copyable {
    constexpr Copyable() noexcept { static_assert(copyable<CRTP>); }
    constexpr Copyable(const Copyable&)            = default;
    constexpr Copyable(Copyable&&)                 = default;
    constexpr Copyable& operator=(const Copyable&) = default;
    constexpr Copyable& operator=(Copyable&&)      = default;
};


} // namespace josh
