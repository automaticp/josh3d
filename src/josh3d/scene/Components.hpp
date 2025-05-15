#pragma once
#include "CategoryCasts.hpp"
#include "Tags.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>


/*
Formally speaking, there's no dedicated `Scene` type. The state of the
scene is fully represented by the contents of some "scene" registry.

This file provides generic tools for dealing with components of all kinds.
*/
namespace josh {


// Component check helper, because `handle.all_of<T>()` is not exactly proper english.
template<typename T>
[[nodiscard]] bool has_component(entt::const_handle handle) noexcept {
    return handle.all_of<T>();
}


// Component check helper, because `handle.all_of<Ts...>()` is not exactly proper english.
template<typename ...Ts>
[[nodiscard]] bool has_components(entt::const_handle handle) noexcept {
    return handle.all_of<Ts...>();
}


// Get a component or evaluate a function to create one.
template<typename T>
auto get_or_create(entt::handle handle, auto&& create_func)
    -> T&
{
    if (auto* comp = handle.try_get<T>()) {
        return *comp;
    } else {
        return handle.emplace<T>(create_func());
    }
}


// Nicer flow for when you want to use designated initializers.
template<typename T>
auto insert_component(entt::handle handle, T&& component)
    -> decltype(auto)
{
    using component_type = std::remove_cvref_t<T>;
    return handle.emplace<component_type>(FORWARD(component));
}


template<std::copyable ...Ts>
void copy_components(entt::handle destination, entt::const_handle source) {
    using type_list = std::tuple<Ts...>;

    const auto copy = [&]<typename T>(std::type_identity<T>) {
        if constexpr (entity_tag<T>) {
            if (has_tag<T>(source)) {
                set_tag<T>(destination);
            }
        } else {
            if (has_component<T>(source)) {
                destination.emplace_or_replace<T>(source.get<T>());
            }
        }
    };

    [&]<size_t ...I>(std::index_sequence<I...>) {
        // Incredible...
        ((void(I), copy(std::type_identity<std::tuple_element_t<I, type_list>>{})), ...);
    }(std::index_sequence_for<Ts...>());
}


} // namespace josh
