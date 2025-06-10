#pragma once
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include <fmt/core.h>
#include <concepts>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <algorithm>


/*
NOTE: Currently a mess of a header with all kinds of "utility" stuff.
The name needs to be changed someday.
*/
namespace josh {


template<typename T, typename KeyT>
concept has_map_find_interface = requires(T map, KeyT key) {
    { map.find(key) } -> std::same_as<typename T::iterator>;
    { map.end()     } -> std::same_as<typename T::iterator>;
};



// Instead of the ugly:
//
//     if (auto it = map.find(key); it != map.end()) { /* ... */ }
//
// Use the other ugly:
//
//     if (auto* item = try_find(map, key)) { /* ... */ }
//
template<typename T, typename KeyT>
    requires has_map_find_interface<T, std::remove_cvref_t<KeyT>>
auto try_find(T& map, KeyT&& key)
    -> T::pointer
{
    if (auto it = map.find(key); it != map.end()) {
        return &(*it);
    } else {
        return decltype(&(*it))(nullptr);
    }
}


template<typename T, typename KeyT>
    requires has_map_find_interface<T, std::remove_cvref_t<KeyT>>
auto try_find(const T& map, KeyT&& key)
    -> T::const_pointer
{
    if (auto it = map.find(key); it != map.end()) {
        return &(*it);
    } else {
        return decltype(&(*it))(nullptr);
    }
}


template<typename T, typename KeyT>
    requires has_map_find_interface<T, std::remove_cvref_t<KeyT>>
auto try_find_value(T& map, KeyT&& key)
    -> T::mapped_type*
{
    if (auto it = map.find(key); it != map.end()) {
        return &(it->second);
    } else {
        return decltype(&(it->second))(nullptr);
    }
}


template<typename T, typename KeyT>
    requires has_map_find_interface<T, std::remove_cvref_t<KeyT>>
auto try_find_value(const T& map, KeyT&& key)
    -> const T::mapped_type*
{
    if (auto it = map.find(key); it != map.end()) {
        return &(it->second);
    } else {
        return decltype(&(it->second))(nullptr);
    }
}




// NOTE: It's important to `delete` this overload, *not*
// reject it a'la SFINAE. If this is SFINAE'd out instead,
// then the other overloads will be picked.
//
// That would be bad since then rvalues can bind as temporaries
// to `const T&` and return a dangling pointer to a subobject.
template<typename T, typename KeyT>
    requires has_map_find_interface<std::remove_cvref_t<T>, std::remove_cvref_t<KeyT>>
auto try_find(T&& rvalue, KeyT&& key) = delete;


template<typename T, typename KeyT>
    requires has_map_find_interface<std::remove_cvref_t<T>, std::remove_cvref_t<KeyT>>
auto try_find_value(T&& rvalue, KeyT&& key) = delete;





template<typename T>
concept has_optional_interface = requires(T opt) {
    { bool(opt) };
    { *opt };
};


// Enables this pattern:
//
//     if (auto* value = try_get(optional)) { /* ... */ }
//
template<has_optional_interface T>
auto try_get(T& opt)
    -> auto*
{
    using value_type = std::remove_cvref_t<decltype(*opt)>;
    return opt ? &(*opt) : static_cast<value_type*>(nullptr);
}


template<has_optional_interface T>
auto try_get(const T& opt)
    -> const auto*
{
    using value_type = std::remove_cvref_t<decltype(*opt)>;
    opt ? &(*opt) : static_cast<const value_type*>(nullptr);
}


template<has_optional_interface T>
auto try_get(T&& rvalue) = delete;


// Moves the value out and resets the optional to default-constructed state.
template<has_optional_interface T>
auto move_out(T& opt) {
    auto value = std::move(*opt);
    opt = {};
    return std::move(value);
}


template<has_optional_interface T>
auto move_out(T&&) = delete;




template<typename T, typename AltT>
concept has_variant_get_interface = requires(T var) {
    get<AltT>(var);
};


template<typename T, typename AltT>
concept has_variant_alternative_interface = requires(T var) {
    { holds_alternative<AltT>(var) } -> std::same_as<bool>;
};


// Moves the value out and resets the variant to default-constructed state.
// This is best used when the variant has some sentinel "null" type as the first alternative.
template<typename AlternativeT, has_variant_get_interface<AlternativeT> T>
auto move_out(T& variant) {
    auto value = std::move(get<AlternativeT>(variant));
    variant = {};
    return std::move(value);
}


template<typename AlternativeT, has_variant_alternative_interface<AlternativeT> T>
bool is(const T& variant) noexcept {
    return holds_alternative<AlternativeT>(variant);
}



// Discard/destroy any movable type by creating a scope, moving
// the object there and closing the scope right after.
template<forwarded_as_rvalue T>
void discard(T&& object) noexcept(std::is_nothrow_move_constructible_v<std::remove_cvref_t<T>>) {
    auto _ = static_cast<std::remove_reference_t<T>&&>(object);
}


namespace detail{
template<typename From>
struct DeferredExplicit {
    From from;
    template<typename To>
    constexpr operator To() && { return To(FORWARD(from)); }
};
template<typename Func>
struct DeferredConvert {
    Func func;
    using ret_type = std::invoke_result_t<Func>;
    constexpr operator ret_type() && { return FORWARD(func)(); }
};
} // namespace detail


// Create a wrapper for deferred explicit conversion of the argument
// to the destination type. Useful for emplace-style functions.
template<typename From>
auto defer_explicit(From&& from) noexcept {
    return detail::DeferredExplicit<From>{ FORWARD(from) };
}


template<typename Func>
auto defer_convert(Func&& func) noexcept {
    return detail::DeferredConvert<Func>(FORWARD(func));
}


template<typename T>
concept has_pop_back_interface = requires(T container) {
    { container.back()     } -> std::same_as<typename T::reference>;
    { container.pop_back() };
};


template<typename T>
concept has_pop_front_interface = requires(T container) {
    { container.front()     } -> std::same_as<typename T::reference>;
    { container.pop_front() };
};


template<typename T>
concept has_pop_queue_interface = requires(T queue_like) {
    { queue_like.front() } -> std::same_as<typename T::reference>;
    ( queue_like.pop()   );
};


// `pop_back()` that actually returns a value.
template<has_pop_back_interface T>
auto pop_back(T& container)
    -> std::remove_cvref_t<T>::value_type
{
    using value_type = std::remove_cvref_t<T>::value_type;
    value_type value = std::move(container.back());
    container.pop_back();
    return value;
}


// `pop_front()` that actually returns a value.
template<has_pop_front_interface T>
auto pop_front(T& container)
    -> std::remove_cvref_t<T>::value_type
{
    using value_type = std::remove_cvref_t<T>::value_type;
    value_type value = std::move(container.front());
    container.pop_front();
    return value;
}


// `pop()` that actually returns a value.
template<has_pop_queue_interface T>
auto pop(T& queue_like)
    -> std::remove_cvref_t<T>::value_type
{
    using value_type = std::remove_cvref_t<T>::value_type;
    value_type value = std::move(queue_like.front());
    queue_like.pop();
    return value;
}


/*
Ever been bothered by how `std::integral_constant` makes you repeat the type?
Or by how it's an *integral*_constant, even though it totaly works with all NTTPs?
Be bothered no more.
*/
template<auto V> struct value_constant : std::integral_constant<decltype(V), V> {};


template<typename ...Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};


namespace detail {
struct NullaryInvoker {
    auto operator%(auto&& f) const -> decltype(auto) { return f(); }
};
} // namespace detail


/*
Convenience for immediately invoked lambda expressions.
The sole purpose is to not have an ugly trailing `()`.
*/
constexpr detail::NullaryInvoker eval = {};


namespace detail {
template<size_t N>
struct Expander {
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


struct BSearchResult {
    size_t prev_idx{};
    size_t next_idx{};
    float  s       {}; // Interpolation coefficient.
};


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
        template<same_as_remove_cvref<Base> T>                \
        requires std::copy_constructible<std::decay_t<T>>     \
        Name(const T& base)                                   \
            : Base{ base }                                    \
        {}                                                    \
        template<same_as_remove_cvref<Base> T>                \
        requires std::move_constructible<std::decay_t<T>> and \
            not_move_or_copy_constructor_of<Name, T>          \
        Name(T&& base) noexcept                               \
            : Base{ MOVE(base) }                              \
        {}                                                    \
    }



// Searches a *sorted* random-access sequence `range` for a `value`.
//
// If `(value <= grid[0])`        returns `prev = next = 0`        and `s = 0.0`;
// If `(value >  grid[size - 1])` returns `prev = next = size - 1` and `s = 1.0`;
//
// Otherwise returns prev and next indices of two neighboring values and
// a linear interpolation coefficient `s` such that `value == (1 - s) * range[prev] + s * range[next]`.
template<typename T>
auto binary_search(
    std::ranges::random_access_range auto&& range,
    const T&                                value) noexcept
        -> BSearchResult
{
    const auto size  = std::ranges::size(range);
    const auto first = range.begin();
    const auto last  = range.end();
    // This returns an iterator pointing to the first element in the range [first, last)
    // such that (element < value) is false, or last if no such element is found.
    const auto next  = std::ranges::lower_bound(range, value, {}, {});

    // NOTE: Order of checks here matters. Handle "first" first, as otherwise
    // an empty range will have you return `size - 1`, which is meaningless.
    if (next == first) {
        return {
            .prev_idx = 0,
            .next_idx = 0,
            .s        = 0.0f
        };
    } else if (next == last) {
        return {
            .prev_idx = size - 1,
            .next_idx = size - 1,
            .s        = 1.0f
        };
    } else [[likely]] {
        const auto prev = next - 1;
        const T& prev_value = *prev;
        const T& next_value = *next;
        const T  diff = next_value - prev_value;

        return {
            .prev_idx = size_t(prev - first),
            .next_idx = size_t(next - first),
            .s        = float((value - prev_value) / diff)
        };
    }
}


/*
Panics by throwing a logic_error. Does not cause UB.
TODO: Deprecate in favor of panic().
*/
[[noreturn]]
inline void safe_unreachable() noexcept(false)
{
    throw std::logic_error("Reached unreachable.");
}

/*
Panics by thowing a logic_error with a custom message. Does not cause UB.
TODO: Deprecate in favor of panic().
*/
[[noreturn]]
inline void safe_unreachable(const char* message) noexcept(false)
{
    throw std::logic_error(message);
}

/*
Panics by throwing a logic_error with an optional custom message. Does not cause UB.
*/
[[noreturn]]
inline void panic(const char* message = nullptr) noexcept(false)
{
    throw std::logic_error(message ? message : "Panic.");
}

/*
Panics by throwing a logic_error with a formatted message. Does not cause UB.
*/
template<typename ...Args>
[[noreturn]]
void panic_fmt(fmt::format_string<Args...> msg, Args&&... args)
{
    throw std::logic_error(fmt::format(msg, FORWARD(args)...));
}

} // namespace josh
