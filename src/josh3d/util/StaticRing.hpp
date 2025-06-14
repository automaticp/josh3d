#pragma once
#include "CategoryCasts.hpp"
#include "Scalars.hpp"
#include <array>


namespace josh {


template<typename T, usize N>
class StaticRing
{
public:
    constexpr StaticRing(std::array<T, N> elements) noexcept
        : storage_{ MOVE(elements) }
    {}

    template<typename ...Ts> requires (sizeof...(Ts) == N)
    constexpr StaticRing(Ts&&... elements) noexcept
        : storage_{ FORWARD(elements)... }
    {}

    constexpr auto current()       noexcept ->       T& { return storage_[current_]; }
    constexpr auto current() const noexcept -> const T& { return storage_[current_]; }
    constexpr auto next()          noexcept ->       T& { return storage_[next_];    }
    constexpr auto next()    const noexcept -> const T& { return storage_[next_];    }

    constexpr void advance() noexcept
    {
        current_ = (current_ + 1) % storage_.size();
        next_    = (next_    + 1) % storage_.size();
    }

    constexpr auto size() const noexcept -> usize { return storage_.size(); }

private:
    std::array<T, N> storage_;
    uindex           current_ = 0;
    uindex           next_    = 1;
};


} // namespace josh
