#include "FrustumCulling.hpp"
#include "Active.hpp"
#include "Camera.hpp"
#include "GeometryCollision.hpp"
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

    for (auto [entity, world_mtf, bounding_sphere] : cullable.each()) {
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

        const float scaled_radius = bounding_sphere.scaled_radius(mesh_scaling);

        const Sphere sphere{ sphere_center, scaled_radius };

        const bool should_be_culled =
            is_fully_outside_of(sphere, frustum_world);

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

        const bool should_be_culled =
            is_fully_outside_of(aabb, frustum_world);

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

