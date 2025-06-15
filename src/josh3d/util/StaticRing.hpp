#pragma once
#include "CategoryCasts.hpp"
#include "Scalars.hpp"
#include <array>


namespace josh {


template<typename T, usize N>
struct StaticRing
{
    constexpr StaticRing() = default;

    constexpr StaticRing(std::array<T, N> elements) noexcept
        : _storage{ MOVE(elements) }
    {}

    template<typename ...Ts> requires (sizeof...(Ts) == N)
    constexpr StaticRing(Ts&&... elements) noexcept
        : _storage{ FORWARD(elements)... }
    {}

    constexpr auto current()       noexcept ->       T& { return _storage[_current]; }
    constexpr auto current() const noexcept -> const T& { return _storage[_current]; }
    constexpr auto next()          noexcept ->       T& { return _storage[_next];    }
    constexpr auto next()    const noexcept -> const T& { return _storage[_next];    }

    constexpr void advance() noexcept
    {
        _current = (_current + 1) % size();
        _next    = (_next    + 1) % size();
    }

    constexpr auto size() const noexcept -> usize { return _storage.size(); }

    std::array<T, N> _storage;
    uindex           _current = 0;
    uindex           _next    = 1;
};


} // namespace josh
