#pragma once
#include "NumericLimits.hpp"
#include "Scalars.hpp"
#include "CommonConcepts.hpp"
#include "CategoryCasts.hpp"
#include <cassert>
#include <concepts>
#include <type_traits>
#include <utility>


/*
We like to have a little bit of everything.

Various utilities with no particular place to be.
*/
namespace josh {


/*
Ever been bothered by how `std::integral_constant` makes you repeat the type?
Or by how it's an *integral*_constant, even though it totaly works with all NTTPs?
Be bothered no more.
*/
template<auto V>
struct value_constant : std::integral_constant<decltype(V), V> {};


/*
Create overload sets on-the-fly and with no commitment. Call today.
*/
template<typename ...Ts>
struct overloaded : Ts... { using Ts::operator()...; };


namespace detail {
struct NullaryInvoker
{
    auto operator%(auto&& f) const -> decltype(auto) { return f(); }
};
} // namespace detail

/*
Convenience for immediately invoked lambda expressions.
The sole purpose is to not have an ugly trailing `()`.
Now you have an ugly leading `eval%` instead. Just better.

FIXME: Why did I do this? This is not sane.
*/
constexpr detail::NullaryInvoker eval = {};


namespace detail {
template<usize N>
struct Expander
{
    constexpr auto operator%(auto&& f) const -> decltype(auto)
    {
        return f(std::make_index_sequence<N>());
    }
};
} // namespace detail

/*
Emulation of expansion statements/expressions.
This comes up a lot when dealing with packs.
*/
#define EXPAND(I, N, ...) \
    ::josh::detail::Expander<N>() % [__VA_ARGS__]<size_t ...I>(std::index_sequence<I...>)


/*
Get the smallest unsigned type that can represent an index up to size of N.
*/
template<usize N> struct smallest_uindex;
template<usize N> requires (N > 0                    and N - 1 <= usize(vmax<u8>))  struct smallest_uindex<N> { using type = u8;  };
template<usize N> requires (N > 1 + usize(vmax<u8>)  and N - 1 <= usize(vmax<u16>)) struct smallest_uindex<N> { using type = u16; };
template<usize N> requires (N > 1 + usize(vmax<u16>) and N - 1 <= usize(vmax<u32>)) struct smallest_uindex<N> { using type = u32; };
template<usize N> requires (N > 1 + usize(vmax<u32>) and N - 1 <= usize(vmax<u64>)) struct smallest_uindex<N> { using type = u64; };
template<usize N> using smallest_uindex_t = smallest_uindex<N>::type;


/*
Ceiling division.
PRE: numer >= 0 and denom > 0
*/
auto div_up(std::integral auto numer, std::integral auto denom) noexcept
{
    assert(numer >= 0);
    assert(denom > 0);
    auto res = numer / denom;
    if (numer % denom) res += 1;
    return res;
}

/*
Useful when rounding up to boundaries.
PRE: number > 0 and start >= 0
*/
auto next_multiple_of(std::integral auto number, std::integral auto start) noexcept
{
    assert(numer > 0);
    assert(start >= 0);
    return number * div_up(start, number);
}


/*
Create a new *unique* type by deriving from an existing one.

For most uses, JOSH3D_DERIVE_TYPE() is recommended instead,
but this can be used to avoid the closing brace and define
additional member funcitions for the derived type.

JOSH3D_DERIVE_TYPE_EX() is similar, but screws with linters
that cannot handle having one brace inside a macro.

WARNING: You should not add non-static data members.
*/
#define JOSH3D_DERIVED_TYPE_BODY(Name, ...)               \
    using Base = __VA_ARGS__;                             \
    using Base::Base;                                     \
    template<same_as_remove_cvref<Base> B>                \
    requires std::copy_constructible<decay_t<B>>          \
    Name(const B& base) : Base{ base } {}                 \
    template<same_as_remove_cvref<Base> B>                \
    requires std::move_constructible<decay_t<B>> and      \
        not_move_or_copy_constructor_of<Name, B>          \
    Name(B&& base) noexcept : Base{ MOVE(base) } {}       \

/*
Create a new *unique* type by deriving from an existing one.

Unlike JOSH3D_DERIVE_TYPE() this does not force a closing brace
at the end, which lets you extend the derived type with additional
member functions.

WARNING: You should not add non-static data members.
*/
#define JOSH3D_DERIVE_TYPE_EX(Name, ...)                      \
    struct Name : __VA_ARGS__                                 \
    {                                                         \
        JOSH3D_DERIVED_TYPE_BODY(Name, __VA_ARGS__)

/*
Create a new *unique* type by deriving from an existing one.

Not every HashMap<UUID, Path> is a ResourceFileTable,
but every ResourceFileTable is a HashMap<UUID, Path>.

NOTE: Might not get every semantic detail right 100%.
*/
#define JOSH3D_DERIVE_TYPE(Name, ...)        \
    JOSH3D_DERIVE_TYPE_EX(Name, __VA_ARGS__) \
    }


} // namespace josh
