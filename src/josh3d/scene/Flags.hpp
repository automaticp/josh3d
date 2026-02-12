#pragma once
#include "ECS.hpp"
#include "Tags.hpp"


/*
Formally speaking, there's no dedicated `Scene` type. The state of the
scene is fully represented by the contents of some "scene" registry.

"Flags" are built on top of "Tags" to provide equivalent iteration performance
for both "tagged" and "not tagged" sets. This is done by emplacing a "negative" tag
`Not<Tag>` for each primary `Tag`, thus creating the storage for the negative set.

Use `registry.view<Not<Tag>, ...>()` instead of `registry.view<...>(exclude<Tag>)`
to take advantage of negative set storage.
*/
namespace josh {


/*
HMM: Probably don't use `Not<Tag>` as the tag itself when
calling set/unset/switch/flag, etc...
*/
template<entity_tag Tag>
struct Not {};

/*
Transform a tagged entity into a flagged entity.
If the entity has `Tag` set, no change is made,
otherwise the `Not<Tag>` is ensured for this entity.
Returns the boolean state corresponding to Tag.
*/
template<entity_tag Tag>
auto flag_by_tag(Handle handle)
    -> bool
{
    if (has_tag<Tag>(handle))
        return true;

    set_tag<Not<Tag>>(handle);
    return false;
}

// TODO: set unset switch


} // namespace josh
