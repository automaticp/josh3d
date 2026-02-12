#pragma once
#include "Scalars.hpp"
#include <range/v3/view/enumerate.hpp> // IWYU pragma: export
#include <range/v3/view/zip.hpp>       // IWYU pragma: export
#include <ranges>                      // IWYU pragma: export
#include <type_traits>


/*
Because I'm done typing this out again and again.
*/
namespace josh {

/*
NOTE: Constants instead of `using` because the linter then has "constant" color.
*/
constexpr auto enumerate = ranges::views::enumerate;
constexpr auto zip       = ranges::views::zip;
constexpr auto transform = std::views::transform;
constexpr auto reverse   = std::views::reverse;

/*
Similar to `views::iota()`, but actually compiles the first time around.

Analogous to Python's `range(beg, end)`, represents a half-open index range: `[beg, end)`.
*/
template<typename T = usize>
constexpr auto irange(std::type_identity_t<T> beg, T end) noexcept
    -> std::ranges::iota_view<T, T>
{
    return std::views::iota(beg, end);
}

/*
Similar to `views::iota()`, but actually compiles the first time around.

Note that this single argument version specifies an *upper* bound, unlike
`views::iota()` where single argument would be the starting index.
Hence the name difference.

Analogous to Python's `range(n)`, represents a half-open index range: `[0, n)`.
*/
template<typename T = usize>
constexpr auto irange(T n) noexcept
    -> std::ranges::iota_view<T, T>
{
    return irange(0, n);
}


} // namespace josh
