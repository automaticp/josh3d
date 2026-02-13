#pragma once
#include "Scalars.hpp"
#include <cassert>
#include <cmath>
#include <numbers>
#include <range/v3/view/generate_n.hpp>
#include <ranges>


namespace josh {

inline auto gaussian_cdf(float x) noexcept -> float
{
    return (1.f + std::erf(x / std::numbers::sqrt2_v<float>)) / 2.f;
}

/*
Uniformly samples the unnormalized gaussian from x0 to x1.

Does not preserve the sum as the tails are not accounted for.
Accounting for tails can make them biased during sampling.
Does not renormalize the resulting bins.

PRE: `right_edge > left_edge`.
*/
inline auto generator_of_binned_gaussian_no_tails(
    float left_edge,
    float right_edge,
    usize num_bins) noexcept
        -> std::ranges::input_range auto
{
    assert(right_edge > left_edge);

    const float dx           = (right_edge - left_edge) / float(num_bins);
    float       current_x    = left_edge;
    float       previous_cdf = gaussian_cdf(left_edge);

    return ranges::views::generate_n(
        [=]() mutable
        {
            current_x += dx;
            const float current_cdf = gaussian_cdf(current_x);
            const float diff        = current_cdf - previous_cdf;
            previous_cdf = current_cdf;
            return diff;
        },
        num_bins);
}



} // namespace josh
