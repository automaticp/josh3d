#pragma once
#include <range/v3/view/enumerate.hpp> // IWYU pragma: export
#include <range/v3/view/zip.hpp>       // IWYU pragma: export
#include <ranges>                      // IWYU pragma: export


/*
Because I'm done typing this out again and again.
*/
namespace josh {


using ranges::views::enumerate;
using ranges::views::zip;
using std::views::transform;


// Similar to `views::iota()`, but actually compiles the first time around.
//
// Analogous to Python's `range(beg, end)`, represents a half-open index range: `[beg, end)`.
constexpr auto irange(size_t beg, size_t end) noexcept
    -> std::ranges::iota_view<size_t, size_t>
{
    return std::views::iota(beg, end);
}


// Similar to `views::iota()`, but actually compiles the first time around.
//
// Note that this single argument version specifies an *upper* bound, unlike
// `views::iota()` where single argument would be the starting index.
// Hence the name difference.
//
// Analogous to Python's `range(n)`, represents a half-open index range: `[0, n)`.
constexpr auto irange(size_t n) noexcept
    -> std::ranges::iota_view<size_t, size_t>
{
    return irange(0, n);
}


} // namespace josh
