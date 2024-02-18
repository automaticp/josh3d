#pragma once
#include "Mesh.hpp"
#include "RenderEngine.hpp"
#include "tags/Culled.hpp"
#include "components/BoundingSphere.hpp"
#include "components/ChildMesh.hpp"
#include "Transform.hpp"
#include "ViewFrustum.hpp"
#include "ECSHelpers.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glm/ext/scalar_common.hpp>
#include <algorithm>
#include <utility>




namespace josh::stages::precompute {


class FrustumCulling {
public:
    void operator()(const RenderEnginePrecomputeInterface& engine, entt::registry& registry);

private:
    template<typename CullTagT = tags::Culled>
    void cull_from_bounding_spheres(entt::registry& registry, const ViewFrustumAsPlanes& frustum);
};




inline void FrustumCulling::operator()(
    const RenderEnginePrecomputeInterface& engine,
    entt::registry& registry)
{
    cull_from_bounding_spheres<tags::Culled>(registry, engine.camera().get_frustum_as_planes());
}




template<typename CullTagT>
void FrustumCulling::cull_from_bounding_spheres(
    entt::registry& registry,
    const ViewFrustumAsPlanes& frustum)
{
    // Assume the frustum has been correctly
    // transformed with the camera's transforms into world-space.

    auto cullable_meshes_view =
        std::as_const(registry).view<Mesh, Transform, components::BoundingSphere>();

    for (auto [entity, _, transform, sphere] : cullable_meshes_view.each()) {

        // FIXME: This seems to be currently broken for the Meshes with non-uniform scaling.
        // Most likely, when the objects are stretched along axis that do not belong
        // to the local basis of the Mesh.
        //
        // How does that even happen?
        // Investigate later, this needs to be rewritten anyway.
        const MTransform world_mtf =
            get_full_mesh_mtransform({ registry, entity }, transform.mtransform());

        const glm::vec3 sphere_center = world_mtf.decompose_position();
        const glm::vec3 mesh_scaling  = world_mtf.decompose_local_scale();

        const float scaled_radius = sphere.scaled_radius(mesh_scaling);

        auto is_fully_in_front_of = [&](const Plane& plane) {
            // Deliniates the enclosing volume of valid positions for the sphere center.
            const float closest_approach = plane.closest_distance + scaled_radius;

            // Projection of the sphere center onto the normal axis of the plane.
            const float normally_projected_distance = glm::dot(plane.normal, sphere_center);

            // This allows us to work with the distance along the normal axis of the plane.
            return normally_projected_distance > closest_approach;
        };

        const bool should_be_culled =
            is_fully_in_front_of(frustum.near())   ||
            is_fully_in_front_of(frustum.far())    ||
            is_fully_in_front_of(frustum.left())   ||
            is_fully_in_front_of(frustum.right())  ||
            is_fully_in_front_of(frustum.bottom()) ||
            is_fully_in_front_of(frustum.top());

        const bool was_culled = registry.all_of<CullTagT>(entity);

        // TODO: Add support for additive culling?
        if (should_be_culled != was_culled) {
            if (should_be_culled) /* as it wasn't previously */ {
                registry.emplace<CullTagT>(entity);
            } else /* should now be un-culled */ {
                registry.erase<CullTagT>(entity);
            }
        }


    }

}





} // namespace josh::stages::precompute
