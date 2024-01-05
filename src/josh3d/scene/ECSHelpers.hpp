#pragma once
#include "Transform.hpp"
#include "components/ChildMesh.hpp"
#include <entt/entt.hpp>



namespace josh {


// Late bound parent transform application.
// Consider inverting your view filtering logic as an alternative.
inline Transform get_full_mesh_transform(
    entt::const_handle mesh_handle,
    const Transform& mesh_transform) noexcept
{
    if (auto as_child = mesh_handle.try_get<components::ChildMesh>()) {
        const Transform& parent_transform =
            mesh_handle.registry()->get<Transform>(as_child->parent);
        return parent_transform * mesh_transform;
    } else {
        return mesh_transform;
    }
}


// Iterate through the view to find it's exact size.
// Cost is O(N) in the size of the view.
inline size_t calculate_view_size(auto entt_view) noexcept {
    size_t count{ 0 };
    for (auto  _ [[maybe_unused]] : entt_view.each()) { ++count; }
    return count;
}


// Removes a tag from the entity if it has one,
// adds a tag to the entity if it doesn't.
// Returns true if the tag was added, false if it was removed.
template<typename TagT>
inline bool switch_tag(entt::handle handle) noexcept {
    if (handle.any_of<TagT>()) {
        handle.remove<TagT>();
        return false;
    } else {
        handle.emplace<TagT>();
        return true;
    }
}



} // namespace josh
