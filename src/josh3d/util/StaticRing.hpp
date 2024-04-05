#pragma once
#include <array>


namespace josh {


template<typename T, size_t N>
class StaticRing {
public:
    constexpr StaticRing(std::array<T, N> elements) noexcept
        : storage_{ std::move(elements) }
    {}

    constexpr       T& current()       noexcept { return storage_[current_]; }
    constexpr const T& current() const noexcept { return storage_[current_]; }

    constexpr       T& next()          noexcept { return storage_[next_];    }
    constexpr const T& next()    const noexcept { return storage_[next_];    }

    constexpr void advance() noexcept {
        current_ = (current_ + 1) % storage_.size();
        next_    = (next_    + 1) % storage_.size();
    }

    constexpr size_t size() const noexcept { return storage_.size(); }

private:
    std::array<T, N> storage_;
    size_t           current_ = 0;
    size_t           next_    = 1;
};


} // namespace josh
