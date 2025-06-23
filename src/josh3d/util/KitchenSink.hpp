#pragma once
#include "Scalars.hpp"
#include "CommonConcepts.hpp"
#include "CategoryCasts.hpp"
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
template<auto V> struct value_constant : std::integral_constant<decltype(V), V> {};


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
*/
constexpr detail::NullaryInvoker eval = {};


namespace detail {
template<usize N>
struct Expander
{
    constexpr auto operator%(auto&& f) const
        -> decltype(auto)
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
Create a new *unique* type by deriving from an existing one.

Not every HashMap<UUID, Path> is a ResourceFileTable,
but every ResourceFileTable is a HashMap<UUID, Path>.

NOTE: Might not get every semantic detail right 100%.
*/
#define JOSH3D_DERIVE_TYPE(Name, ...)                         \
    struct Name : __VA_ARGS__                                 \
    {                                                         \
        using Base = __VA_ARGS__;                             \
        using Base::Base;                                     \
        template<same_as_remove_cvref<Base> B>                \
        requires std::copy_constructible<std::decay_t<B>>     \
        Name(const B& base) : Base{ base } {}                 \
        template<same_as_remove_cvref<Base> B>                \
        requires std::move_constructible<std::decay_t<B>> and \
            not_move_or_copy_constructor_of<Name, B>          \
        Name(B&& base) noexcept : Base{ MOVE(base) } {}       \
    }


} // namespace josh
