#include "FrustumCulling.hpp"
#include "Active.hpp"
#include "Camera.hpp"
#include "Tags.hpp"
#include "RenderEngine.hpp"
#include "tags/Culled.hpp"
#include "BoundingSphere.hpp"
#include "Transform.hpp"
#include "ViewFrustum.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/ext/scalar_common.hpp>
#include <utility>




namespace josh::stages::precompute {
namespace {


template<typename CullTagT>
void cull_from_bounding_spheres(
    entt::registry&            registry,
    const ViewFrustumAsPlanes& frustum_world)
{
    using glm::vec3;

    auto cullable = std::as_const(registry).view<MTransform, BoundingSphere>();

    for (auto [entity, world_mtf, sphere] : cullable.each()) {
        const entt::handle handle{ registry, entity };

        // FIXME: This seems to be currently broken for the Meshes with non-uniform scaling.
        // Most likely, when the objects are stretched along axis that do not belong
        // to the local basis of the Mesh.
        //
        // How does that even happen?
        // Investigate later, this needs to be rewritten anyway.

        // FIXME: PointLights need special handling where the scaling is discarded.
        // TODO: Or we need to interpret BoundingSphere as already scaled to world (better).

        const vec3 sphere_center = world_mtf.decompose_position();
        const vec3 mesh_scaling  = world_mtf.decompose_local_scale();

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
            is_fully_in_front_of(frustum_world.near())   ||
            is_fully_in_front_of(frustum_world.far())    ||
            is_fully_in_front_of(frustum_world.left())   ||
            is_fully_in_front_of(frustum_world.right())  ||
            is_fully_in_front_of(frustum_world.bottom()) ||
            is_fully_in_front_of(frustum_world.top());

        if (should_be_culled) {
            set_tag<CullTagT>(handle);
        }
    }
}


template<typename CullTagT = Culled>
void cull_from_aabbs(
    entt::registry&            registry,
    const ViewFrustumAsPlanes& frustum_world)
{
    using glm::vec3;

    auto cullable = std::as_const(registry).view<AABB>();

    for (auto [entity, aabb] : cullable.each()) {
        const entt::handle handle{ registry, entity };

        auto is_fully_in_front_of = [&](const Plane& plane) {

            // Here quadrant of the normal is irrelevant, since the AABB extents
            // are symmetric wrt. to reflection around the midpoint.
            //
            // This, effectively, "selects" the closest vertex,
            // and computes the distance from it to the midpoint,
            // projected along the plane normal.
            //
            // Note that this selected vertex is really "closest" only
            // if it is in front of the plane. But the same vertex
            // is "selected" even if the AABB is intersecting, or behind the plane.
            const float projected_extent = dot((aabb.extents() / 2), abs(plane.normal));

            // Projected midpoint in world. Or closest distance from the plane
            // that is oriented by the same normal, but goes through the world origin.
            const float projected_midpoint = dot(aabb.midpoint(), plane.normal);

            // Along the plane normal, the following holds:
            //
            //      projected_midpoint =
            //          plane.closest_distance     +
            //          distance_to_closest_vertex +
            //          projected_extent
            //
            // where `distance_to_closest_vertex` is the closest distance
            // between the plane and the "closest" vertex.
            //
            // If that distance is positive, the "closest" vertex is
            // in front of the plane.
            const float distance_to_closest_vertex =
                (projected_midpoint - plane.closest_distance) - projected_extent;

            // TODO: There seems to be incorrect culling when above a plane-like object
            // and looking up ~40 degrees. Debug when frustum visualization is available.

            return distance_to_closest_vertex > 0.f;
        };

        const bool should_be_culled =
            is_fully_in_front_of(frustum_world.near())   ||
            is_fully_in_front_of(frustum_world.far())    ||
            is_fully_in_front_of(frustum_world.left())   ||
            is_fully_in_front_of(frustum_world.right())  ||
            is_fully_in_front_of(frustum_world.bottom()) ||
            is_fully_in_front_of(frustum_world.top());

        if (should_be_culled) {
            set_tag<Culled>(handle);
        }

    }
}


} // namespace




void FrustumCulling::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    if (const auto camera = get_active<Camera, MTransform>(engine.registry())) {

        engine.registry().clear<Culled>();

        const auto frustum_local = camera.get<Camera>().view_frustum_as_planes();
        const auto frustum_world = frustum_local.transformed(camera.get<MTransform>().model());

        // cull_from_bounding_spheres<Culled>(engine.registry(), frustum_world);
        cull_from_aabbs<Culled>           (engine.registry(), frustum_world);
    }
}


} // namespace josh::stages::precompute

