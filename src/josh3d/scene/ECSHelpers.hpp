#pragma once
#include "Transform.hpp"
#include "components/ChildMesh.hpp"
#include "components/Model.hpp"
#include <entt/entt.hpp>




namespace josh {


// Late bound parent-to-child transform chaining.
// Consider inverting your view filtering logic as an alternative.
inline auto get_full_mesh_mtransform(
    entt::const_handle mesh_handle,
    const MTransform& mesh_mtransform
) noexcept
    -> MTransform
{
    if (auto as_child = mesh_handle.try_get<components::ChildMesh>()) {
        const Transform& parent_transform =
            mesh_handle.registry()->get<Transform>(as_child->parent);
        return parent_transform.mtransform() * mesh_mtransform;
    } else {
        return mesh_mtransform;
    }
}


// Late bound parent-to-child transform chaining.
inline auto get_full_mesh_mtransform(entt::const_handle mesh_handle) noexcept
    -> MTransform
{
    const Transform& mesh_transform = mesh_handle.get<Transform>();

    if (auto as_child = mesh_handle.try_get<components::ChildMesh>()) {
        const Transform& parent_transform =
            mesh_handle.registry()->get<Transform>(as_child->parent);
        return parent_transform.mtransform() * mesh_transform.mtransform();
    } else {
        return mesh_transform.mtransform();
    }
}


inline void destroy_model(entt::handle model_handle) noexcept {
    auto& registry = *model_handle.registry();
    auto& model    = model_handle.get<components::Model>();
    registry.destroy(model.meshes().begin(), model.meshes().end());
    model_handle.destroy();
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
