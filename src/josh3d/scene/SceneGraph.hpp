#pragma once
#include "ChildListIterator.hpp"
#include <cstddef>
#include <entt/entity/component.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/entity.hpp>
#include <cassert>
#include <ranges>


/*
Formally speaking, there's no dedicated `Scene` type. The state of the
scene is fully represented by the contents of some "scene" registry.

This file defines parent-child relationships between entities that establish
a "transform" hierarchy. This is your average scene-graph.

It is strongly advised to only use function from this file in order to
modify the hierarchy. Otherwise, all responsibility for preserving the
invariants is on you.

Do note that this hierarchy is not the only relationship graph that each
entity can participate in.
*/
namespace josh {


/*
Relationship component for entities that makes this entity a parent
of some others, that must be `AsChild`.

This relationship implies *only* the transform hierarchy.
*/
struct AsParent {
    std::uint32_t num_children{};
    entt::entity  first_child; // Must exist if you have the component.
};


/*
Relationship component for entities that makes this entity a child
of some `AsParent` entity.

This relationship implies *only* the transform hierarchy.
*/
struct AsChild {
    entt::entity parent; // Must exist if you have the component.
    entt::entity next{ entt::null };
    entt::entity prev{ entt::null };
};


/*
Tag component for deferred detachment of child entities.
*/
struct MarkedForDetachment {};



// TODO: Can this be done?
inline void _enable_orphaning_on_destruction(entt::registry& registry) noexcept;




[[nodiscard]] inline bool has_parent(entt::const_handle handle) noexcept {
    return handle.all_of<AsChild>();
}


[[nodiscard]] inline bool has_children(entt::const_handle handle) noexcept {
    return handle.all_of<AsParent>();
}




[[nodiscard]] inline auto get_parent_entity(entt::const_handle child_handle) noexcept
    -> entt::entity
{
    if (const AsChild* as_child = child_handle.try_get<AsChild>()) {
        return as_child->parent;
    } else {
        return entt::null;
    }
}


// Convinience to not drop the "handle"-ness of operations.
[[nodiscard]] inline auto get_parent_handle(entt::const_handle child_handle) noexcept
    -> entt::const_handle
{
    return { *child_handle.registry(), get_parent_entity(child_handle) };
}


// Convinience to not drop the "handle"-ness of operations.
[[nodiscard]] inline auto get_parent_handle(entt::handle child_handle) noexcept
    -> entt::handle
{
    return { *child_handle.registry(), get_parent_entity(child_handle) };
}




[[nodiscard]] inline auto get_root_entity(entt::const_handle handle) noexcept
    -> entt::entity
{
    auto& registry = *handle.registry();
    entt::entity top = handle.entity();
    while (const AsChild* as_child = registry.try_get<AsChild>(top)) {
        top = as_child->parent;
    }
    return top;
}


[[nodiscard]] inline auto get_root_handle(entt::const_handle handle) noexcept
    -> entt::const_handle
{
    return { *handle.registry(), get_root_entity(handle) };
}


[[nodiscard]] inline auto get_root_handle(entt::handle handle) noexcept
    -> entt::handle
{
    return { *handle.registry(), get_root_entity(handle) };
}




[[nodiscard]] inline auto get_first_child_entity(entt::const_handle parent_handle) noexcept
    -> entt::entity
{
    if (const AsParent* as_parent = parent_handle.try_get<AsParent>()) {
        return as_parent->first_child;
    } else {
        return entt::null;
    }
}


[[nodiscard]] inline auto get_first_child_handle(entt::const_handle parent_handle) noexcept
    -> entt::const_handle
{
    return { *parent_handle.registry(), get_first_child_entity(parent_handle) };
}


[[nodiscard]] inline auto get_first_child_handle(entt::handle parent_handle) noexcept
    -> entt::handle
{
    return { *parent_handle.registry(), get_first_child_entity(parent_handle) };
}




[[nodiscard]] inline auto view_child_entities(entt::const_handle parent_handle) noexcept
    -> child_list_view<entt::entity, entt::const_handle, AsChild>
{
    // Will be null if no children. Therefore, begin() == end(), and the view is empty.
    return { get_first_child_handle(parent_handle) };
}


[[nodiscard]] inline auto view_child_handles(entt::const_handle parent_handle) noexcept
    -> child_list_view<entt::const_handle, entt::const_handle, AsChild>
{
    return { get_first_child_handle(parent_handle) };
}


[[nodiscard]] inline auto view_child_handles(entt::handle parent_handle) noexcept
    -> child_list_view<entt::handle, entt::handle, AsChild>
{
    return { get_first_child_handle(parent_handle) };
}




inline void attach_child(
    entt::handle parent_handle,
    entt::entity new_child) noexcept
{
    const entt::handle new_child_handle{ *parent_handle.registry(), new_child };
    assert(!new_child_handle.all_of<AsChild>() && "Entity already has a parent.");

    auto& as_parent = parent_handle.get_or_emplace<AsParent>(0, entt::null);

    // Attach new entity to the front.
    const entt::entity next = as_parent.first_child;
    const entt::entity prev = entt::null;

    new_child_handle.emplace<AsChild>(parent_handle.entity(), next, prev);

    // Adjust the following entity, if not null.
    if (next != entt::null) {
        parent_handle.registry()->get<AsChild>(next).prev = new_child;
    }

    // Adjust the parent fields.
    as_parent.first_child = new_child;
    ++as_parent.num_children;
}


inline void attach_to_parent(
    entt::handle new_child_handle,
    entt::entity parent) noexcept
{
    return attach_child({ *new_child_handle.registry(), parent }, new_child_handle.entity());
}


// Prepends children from the range.
// No-op when the range is empty.
inline void attach_children(
    entt::handle                    parent_handle,
    std::ranges::input_range auto&& new_child_entities) noexcept
{
    auto& registry = *parent_handle.registry();

    entt::entity head = entt::null;

    // If aleady has children, set the head to the first child.
    if (const AsParent* as_parent = parent_handle.try_get<AsParent>()) {
        head = as_parent->first_child;
    }

    size_t num_attached = 0;

    for (const entt::entity child_entity : new_child_entities) {

        const entt::handle child_handle{ registry, child_entity };
        assert(!child_handle.all_of<AsChild>() && "Entity already has a parent.");

        // Connect this to the current head.
        child_handle.emplace<AsChild>(parent_handle.entity(), head, entt::null);

        // Connect current head to this.
        if (head != entt::null) {
            registry.get<AsChild>(head).prev = child_entity;
        }

        head = child_entity; // Last prepended head will be the new first_child.
        ++num_attached;
    }

    // Adjust the parent if there were any children in the range.
    if (num_attached != 0) {
        auto& as_parent = parent_handle.get_or_emplace<AsParent>(0, entt::null);
        as_parent.num_children += num_attached;
        as_parent.first_child   = head;
    }
}




inline void detach_from_parent(entt::handle child_handle) noexcept {
    assert(child_handle.all_of<AsChild>() && "Entity does not have a parent.");

    auto& registry = *child_handle.registry();
    auto& as_child = child_handle.get<AsChild>();

    const entt::handle parent_handle{ registry, as_child.parent };
    const entt::handle next_handle  { registry, as_child.next   };
    const entt::handle prev_handle  { registry, as_child.prev   };

    // Update immediate neighbors.
    if (next_handle.entity() != entt::null) {
        next_handle.get<AsChild>().prev = prev_handle.entity();
    }

    if (prev_handle.entity() != entt::null) {
        prev_handle.get<AsChild>().next = next_handle.entity();
    }

    // Remove AsChild from detached entity.
    child_handle.erase<AsChild>();

    // Adjust the parent fields.
    auto& parent_as_parent = parent_handle.get<AsParent>();

    if (parent_as_parent.num_children == 1) {
        assert(next_handle.entity() == entt::null && prev_handle.entity() == entt::null); // Sanity check.
        // The only child was detached. Remove AsParent component.
        parent_handle.erase<AsParent>();
    } else {
        // More children remain.
        const bool detached_first_child{ prev_handle.entity() == entt::null };
        if (detached_first_child) {
            parent_as_parent.first_child = next_handle.entity();
        }
        --parent_as_parent.num_children;
    }
}


inline void detach_all_children(entt::handle parent_handle) noexcept {
    assert(parent_handle.all_of<AsParent>());
    auto& registry  = *parent_handle.registry();
    auto& as_parent = parent_handle.get<AsParent>();

    // NOTE: Do not do:
    //
    //   auto children = child_list_view<entt::entity, entt::handle, AsChild>({ registry, as_parent.first_child });
    //   registry.remove<AsChild>(children.begin(), children.end());
    //
    // As this will read from removed memory of AsChild components.
    //
    // This can be made possible if the `child_list_iterator` would store the next and previous entities,
    // instead of referring to the registy component, but it is not implemented like that right now,
    // and it might have its own downsides (like state desync between storage and iterators).

    size_t num_detached{ 0 };
    assert(as_parent.first_child != entt::null);
    entt::entity next = as_parent.first_child;

    do {
        const entt::handle current_handle{ registry, next };
        next = current_handle.get<AsChild>().next;

        current_handle.erase<AsChild>();

        ++num_detached;
    } while (next != entt::null); // If the next is null then we just detached the last one. Done.


    assert(num_detached == as_parent.num_children); // Sanity check.
    parent_handle.erase<AsParent>();
}




inline void mark_for_detachment(entt::handle child_handle) noexcept {
    assert(!child_handle.all_of<MarkedForDetachment>());
    assert(child_handle.all_of<AsChild>());
    child_handle.emplace<MarkedForDetachment>();
}


inline void sweep_marked_for_detachment(entt::registry& registry) noexcept {
    for (const entt::entity& child : registry.view<MarkedForDetachment>()) {
        detach_from_parent({ registry, child });
    }
    registry.clear<MarkedForDetachment>();
}




template<typename FuncT>
concept traversal_function =
    std::invocable<FuncT, entt::handle, ptrdiff_t> ||
    std::invocable<FuncT, entt::handle>;

namespace detail {
// Helper to not spam `if constexpr` everywhere.
template<traversal_function FuncT>
void invoke_traversal_function(FuncT&& function, entt::handle handle, ptrdiff_t depth [[maybe_unused]]) {
    if constexpr (std::invocable<FuncT, entt::handle, ptrdiff_t>) {
        function(handle, depth);
    } else {
        function(handle);
    }
}
} // namespace detail


// Recursively iterate all descendants of `handle` and call a function on them.
// This does not affect the root entity itself.
// The call does nothing if the root entity has no children.
//
// Function signature is: `void function(entt::handle handle, ptrdiff_t depth);`
// Where second argument is passed `(depth + 1)` for direct children, and so forth.
//
// This is a depth-first pre-order traversal.
template<traversal_function FuncT>
inline void traverse_descendants_preorder(
    entt::handle handle,
    FuncT&&      function,
    ptrdiff_t    depth = 0)
{
    for (const entt::handle child_handle : view_child_handles(handle)) {
        detail::invoke_traversal_function(function, child_handle, depth + 1);
        traverse_descendants_preorder(child_handle, function, depth + 1);
    }
}


// Recursively iterate all nodes of a subtree starting at `handle` and call a function on them.
template<traversal_function FuncT>
inline void traverse_subtree_preorder(
    entt::handle handle,
    FuncT&&      function,
    ptrdiff_t    depth = 0)
{
    detail::invoke_traversal_function(function, handle, depth);
    for (const entt::handle child_handle : view_child_handles(handle)) {
        traverse_subtree_preorder(child_handle, function, depth + 1);
    }
}


// Traverse the tree edge from the current node up to the root, *including* the starting node.
//
// `depth` is decremented for each upward step.
template<traversal_function FuncT>
inline void traverse_edge_upwards(
    entt::handle handle,
    FuncT&&      function,
    ptrdiff_t    depth = 0)
{
    do {
        detail::invoke_traversal_function(function, handle, depth);
        handle = get_parent_handle(handle);
        --depth;
    } while (handle.entity() != entt::null); // Signals the past-the-root.
}


// Traverse the tree edge from the current node up to the root, *excluding* the starting node.
// This does nothing if the `handle` has no parents.
//
// `depth` is decremented for each upward step.
template<traversal_function FuncT>
inline void traverse_ancestors_upwards(
    entt::handle handle,
    FuncT&&      function,
    ptrdiff_t    depth = 0)
{
    handle = get_parent_handle(handle);
    while (handle.entity() != entt::null) {
        --depth;
        detail::invoke_traversal_function(function, handle, depth);
        handle = get_parent_handle(handle);
    }
}




} // namespace josh
