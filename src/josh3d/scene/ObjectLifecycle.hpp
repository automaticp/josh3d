#pragma once
#include "SceneGraph.hpp"
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>


/*
Formally speaking, there's no dedicated `Scene` type. The state of the
scene is fully represented by the contents of some "scene" registry.

From EnTT's "Crash Course: entity-component system"
(See https://skypjack.github.io/entt/pages.html - direct link may change):

"""
Most of the ECS available out there don't allow to create and destroy entities and components
during iterations, nor to have pointer stability. EnTT partially solves the problem with a few limitations:

    - Creating entities and components is allowed during iterations in most cases and it never invalidates
    already existing references.

    - Deleting the current entity or removing its components is allowed during iterations but it could
    invalidate references. For all the other entities, destroying them or removing their iterated components
    isn't allowed and can result in undefined behavior.

    - When pointer stability is enabled for the type leading the iteration, adding instances of the same type
    may or may not cause the entity involved to be returned. Destroying entities and components is always allowed
    instead, even if not currently iterated, without the risk of invalidating any references.

    - In case of reverse iterations, adding or removing elements is not allowed under any circumstances.
    It could quickly lead to undefined behaviors.

In other terms, iterators are rarely invalidated. Also, component references aren't invalidated when a new element
is added while they could be invalidated upon destruction due to the swap-and-pop policy, unless the type leading
the iteration undergoes in-place deletion.

...

For all types that don't offer stable pointers, iterators are also invalidated and the behavior is undefined
if an entity is modified or destroyed and it's not the one currently returned by the iterator nor a newly created one.
To work around it, possible approaches are:

    - Store aside the entities and the components to be removed and perform the operations at the end of the iteration.

    - Mark entities and components with a proper tag component that indicates they must be purged, then perform
    a second iteration to clean them up one by one.

A notable side effect of this feature is that the number of required allocations is further reduced in most cases.
"""

The caveats above are the reason for existance of these helpers.
*/
namespace josh {


/*
Tag component for deferred destruction of entities.

This is used to perform cleanup where direct destruction is not possible
due to iterator invalidation, or where that cleanup is complicated for other reasons.
*/
struct MarkedForDestruction {};




inline void mark_for_destruction(entt::handle handle) noexcept {
    assert(!handle.all_of<MarkedForDestruction>());
    handle.emplace<MarkedForDestruction>();
}


inline void sweep_marked_for_destruction(entt::registry& registry) noexcept {
    auto marked = registry.view<MarkedForDestruction>();
    registry.destroy(marked.begin(), marked.end());
}


inline void destroy_subtree(entt::handle handle) noexcept {
    traverse_subtree_preorder(handle, [](entt::handle handle) { mark_for_destruction(handle); });
    if (has_parent(handle)) {
        detach_from_parent(handle);
    }
    sweep_marked_for_destruction(*handle.registry());
}


inline void destroy_and_orphan_children(entt::handle handle) noexcept {
    if (has_children(handle)) {
        detach_all_children(handle);
    }
    if (has_parent(handle)) {
        detach_from_parent(handle);
    }
    handle.destroy();
}



} // namespace josh
