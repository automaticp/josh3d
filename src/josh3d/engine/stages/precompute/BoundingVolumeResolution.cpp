#include "BoundingVolumeResolution.hpp"
#include "RenderEngine.hpp"
#include "Transform.hpp"
#include "BoundingSphere.hpp"
#include "AABB.hpp"
#include "ECS.hpp"


namespace josh::stages::precompute {


void BoundingVolumeResolution::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    auto& registry = engine.registry();

    for (const auto [entity, local_aabb, mtf] :
        registry.view<LocalAABB, MTransform>().each())
    {
        const Handle handle{ registry, entity };
        handle.emplace_or_replace<AABB>(local_aabb.transformed(mtf.model()));
    }

    for (const auto [entity, local_sphere, mtf] :
        registry.view<LocalBoundingSphere, MTransform>().each())
    {
        const Handle handle{ registry, entity };
        handle.emplace_or_replace<BoundingSphere>(local_sphere.transformed(mtf.model()));
    }
}


} // namespace josh::stages::precompute
