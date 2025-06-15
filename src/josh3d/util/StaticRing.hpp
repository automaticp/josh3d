#pragma once
#include "CategoryCasts.hpp"
#include "Scalars.hpp"
#include <array>


namespace josh {


template<typename T, usize N>
struct StaticRing
{
    std::array<T, N> storage;

    constexpr StaticRing() = default;

    constexpr StaticRing(std::array<T, N> elements) noexcept
        : storage{ MOVE(elements) }
    {}

    template<typename ...Ts> requires (sizeof...(Ts) == N)
    constexpr StaticRing(Ts&&... elements) noexcept
        : storage{ FORWARD(elements)... }
    {}

    constexpr auto current()       noexcept ->       T& { return storage[_current]; }
    constexpr auto current() const noexcept -> const T& { return storage[_current]; }
    constexpr auto next()          noexcept ->       T& { return storage[_next];    }
    constexpr auto next()    const noexcept -> const T& { return storage[_next];    }

    constexpr void advance() noexcept
    {
        _current = (_current + 1) % size();
        _next    = (_next    + 1) % size();
    }

    constexpr auto size() const noexcept -> usize { return storage.size(); }

    uindex _current = 0;
    uindex _next    = 1;
};


} // namespace josh
