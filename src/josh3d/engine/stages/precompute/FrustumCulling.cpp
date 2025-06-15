#include "FrustumCulling.hpp"
#include "Active.hpp"
#include "Camera.hpp"
#include "ECS.hpp"
#include "GeometryCollision.hpp"
#include "Tags.hpp"
#include "RenderEngine.hpp"
#include "BoundingSphere.hpp"
#include "Transform.hpp"
#include "ViewFrustum.hpp"
#include "tags/Visible.hpp"
#include <utility>


namespace josh {
namespace {

void cull_from_bounding_spheres(
    Registry&            registry,
    const FrustumPlanes& frustum_world)
{
    auto cullable = std::as_const(registry).view<BoundingSphere>();

    for (auto [entity, bounding_sphere] : cullable.each())
    {
        const Handle handle = { registry, entity };

        const bool should_be_culled =
            is_fully_outside_of(bounding_sphere, frustum_world);

        if (not should_be_culled)
            set_tag<Visible>(handle);
    }
}

void cull_from_aabbs(
    Registry&            registry,
    const FrustumPlanes& frustum_world)
{
    auto cullable = std::as_const(registry).view<AABB>();

    for (auto [entity, aabb] : cullable.each())
    {
        const Handle handle = { registry, entity };

        const bool should_be_culled =
            is_fully_outside_of(aabb, frustum_world);

        if (not should_be_culled)
            set_tag<Visible>(handle);
    }
}

} // namespace


void FrustumCulling::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    if (const auto camera = get_active<Camera, MTransform>(engine.registry()))
    {
        engine.registry().clear<Visible>();

        const auto frustum_local = camera.get<Camera>().view_frustum_as_planes();
        const auto frustum_world = frustum_local.transformed(camera.get<MTransform>().model());

        cull_from_bounding_spheres(engine.registry(), frustum_world);
        cull_from_aabbs           (engine.registry(), frustum_world);
    }
}


} // namespace josh

