#pragma once
#include "ECS.hpp"
#include <type_traits>


/*
Formally speaking, there's no dedicated `Scene` type. The state of the
scene is fully represented by the contents of some "scene" registry.

This file provides support for "tags" as a special type of components that simulates true boolean sets.
This is one of the ECS ways of doing `is_visible`, `is_active`, etc. EnTT optimizes away storage
for "tags" (any empty standard-layout type), so I'd like to communicate tagging more explicitly.

Note that entities with a tag and without a tag are *not equivalent* for their iteration performance.
Iteration over tagged entities is driven solely by the tag storage, while iteration over non-tagged
entities is driven by another storage, with the "no tag" applied as a negative filter.
If both "tagged" and and "not tagged" entities need to be iterated with equivalent performance
consider using Flags instead.
*/
namespace josh {


/*
Tag types must be empty.
This is the condition that makes storage optimization possible.
See also: `entt::component_traits`.
*/
template<typename T>
concept entity_tag = std::is_empty_v<T>;

/*
Check if the entity is tagged with the specified tag.
Equivalent to boolean state.
*/
template<entity_tag TagT>
[[nodiscard]] auto has_tag(CHandle handle) noexcept
    -> bool
{
    return handle.all_of<TagT>();
}

/*
Check if the entity is tagged with all of the specified tags.
*/
template<entity_tag ...TagTs>
[[nodiscard]] auto has_tags(CHandle handle) noexcept
    -> bool
{
    return handle.all_of<TagTs...>();
}

/*
"Tag" an entity, implicitly including it in the "tagged" set.
Returns true if the tag was set successfully, false if it was already set.
*/
template<entity_tag TagT>
auto set_tag(Handle handle) noexcept
    -> bool
{
    if (has_tag<TagT>(handle))
        return false;

    handle.emplace<TagT>();
    return true;
}

/*
Remove the tag from the entity, removing it from the "tagged" set.
Returns true if the tag was removed successfully, false if the entity was not tagged.
*/
template<entity_tag TagT>
auto unset_tag(Handle handle) noexcept
    -> bool
{
    return handle.remove<TagT>();
}

/*
Remove the tag from the entity if it has one, add the tag to the entity if it does not.
Returns true if the tag was added, false if it was removed.
*/
template<entity_tag TagT>
auto switch_tag(Handle handle) noexcept
    -> bool
{
    if (has_tag<TagT>(handle))
    {
        unset_tag<TagT>(handle);
        return false;
    }
    else
    {
        set_tag<TagT>(handle);
        return true;
    }
}


} // namespace josh
