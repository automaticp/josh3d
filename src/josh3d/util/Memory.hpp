#pragma once
#include "CommonConcepts.hpp"
#include <algorithm>
#include <cassert>


namespace josh {

template<typename T>
struct GrowthRatio
{
    T numer;
    T denom;
};

/*
Returns the new size that would grow from `cur_size` with the amortization
ratio of `numer / denom`, such that it is at least `new_size`.

PRE: `new_size > cur_size`.
PRE: `ratio.numer > ratio.denom`.
*/
template<typename T>
[[nodiscard]]
constexpr auto amortized_size_at_least(
    T                           new_size,
    not_deduced<T>              cur_size,
    not_deduced<GrowthRatio<T>> ratio = { 2, 1 }) noexcept
        -> T
{
    assert(ratio.numer > ratio.denom);
    assert(new_size > cur_size);
    const T amortized_size = cur_size * ratio.numer / ratio.denom;
    return std::max(amortized_size, new_size);
}

/*
Returns the new size that would grow from `cur_size` as if by pushing back
a single element to the end of the implied [0, cur_size) range with the
amortization ratio of `numer / denom`.

PRE: `ratio.numer > ratio.denom`.
*/
template<typename T>
[[nodiscard]]
constexpr auto amortized_size_push_one(
    T                           cur_size,
    not_deduced<GrowthRatio<T>> ratio = { 2, 1 }) noexcept
        -> T
{
    return amortized_size_at_least(cur_size + 1, cur_size, ratio);
}


} // namespace josh
