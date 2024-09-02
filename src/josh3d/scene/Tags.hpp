#pragma once
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>
#include <type_traits>


/*
Formally speaking, there's no dedicated `Scene` type. The state of the
scene is fully represented by the contents of some "scene" registry.

This file provides support for "tags" as a special type of components that simulates boolean flags.
This is the ECS way of doing `is_visible`, `is_active`, etc. EnTT optimizes away storage
for "tags" (any empty standard-layout type), so I'd like to communicate tagging more explicitly.
*/
namespace josh {


// Tag types must be empty.
// This is the condition that makes storage optimization possible.
// See also: `entt::component_traits`.
template<typename T>
concept entity_tag = std::is_empty_v<T>;


// Check if the entity is tagged with the specified tag.
// Equivalent to boolean state.
template<entity_tag TagT>
[[nodiscard]] bool has_tag(entt::const_handle handle) noexcept {
    return handle.all_of<TagT>();
}


// Check if the entity is tagged with all of the specified tags.
template<entity_tag ...TagTs>
[[nodiscard]] bool has_tags(entt::const_handle handle) noexcept {
    return handle.all_of<TagTs...>();
}


// "Tag" an entity, setting the implied boolean to "true".
// Returns true if the tag was set successfully, false if it was already set.
template<entity_tag TagT>
bool set_tag(entt::handle handle) noexcept {
    if (has_tag<TagT>(handle)) {
        return false;
    } else {
        handle.emplace<TagT>();
        return true;
    }
}


// Remove the tag from the entity, setting the implied boolean to "false".
// Returns true if the tag was removed successfully, false if the entity was not tagged.
template<entity_tag TagT>
bool unset_tag(entt::handle handle) noexcept {
    return handle.remove<TagT>();
}


// Remove the tag from the entity if it has one, add the tag to the entity if it does not.
// Returns true if the tag was added, false if it was removed.
template<entity_tag TagT>
bool switch_tag(entt::handle handle) noexcept {
    if (has_tag<TagT>(handle)) {
        unset_tag<TagT>(handle);
        return false;
    } else {
        set_tag<TagT>(handle);
        return true;
    }
}




} // namespace josh
