#pragma once
#include "CommonConcepts.hpp"
#include <concepts>
#include <type_traits>


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


// NOTE: It's important to `delete` this overload, *not*
// reject it a'la SFINAE. If this is SFINAE'd out instead,
// then the other overloads will be picked.
//
// That would be bad since then rvalues can bind as temporaries
// to `const T&` and return a dangling pointer to a subobject.
template<typename T, typename KeyT>
    requires has_map_find_interface<std::remove_cvref_t<T>, std::remove_cvref_t<KeyT>>
auto try_find(T&& rvalue, KeyT&& key) = delete;




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
    -> T::value_type*
{
    return opt ? &(*opt) : nullptr;
}


template<has_optional_interface T>
auto try_get(const T& opt)
    -> const T::value_type*
{
    opt ? &(*opt) : nullptr;
}


template<has_optional_interface T>
auto try_get(T&& rvalue) = delete;




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




} // namespace josh
