#include "TransformResolution.hpp"
#include "AABB.hpp"
#include "Transform.hpp"
#include "SceneGraph.hpp"
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>


namespace josh::stages::precompute {


// Will get as deep as the deepest scene-graph tree. So should be pretty shallow.
static void resolve_transforms_recursive(
    const entt::handle& node_handle,
    const MTransform&   node_mtf) noexcept
{
    if (has_children(node_handle)) {
        for (const entt::handle child_handle : view_child_handles(node_handle)) {

            const MTransform child_local_mtf = child_handle.get_or_emplace<Transform>().mtransform();
            const MTransform child_mtf       = node_mtf * child_local_mtf;
            child_handle.emplace_or_replace<MTransform>(child_mtf);

            resolve_transforms_recursive(child_handle, child_mtf);
        }
    }
}




void TransformResolution::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    auto& registry = engine.registry();

    // TODO: Two quirks, that are somewhat contradictory:
    // 1. This only operates on the root nodes that *have* the Transform already.
    // 2. All child nodes that *don't have* the Transform, get a default one emplaced.

    // This resolves the scene-graph.
    for (auto [root_entity, transform] :
        registry.view<Transform>(entt::exclude<AsChild>).each())
    {
        // Update root transform.
        const entt::handle root_handle{ registry, root_entity  };
        const MTransform   root_mtf   = transform.mtransform();
        root_handle.emplace_or_replace<MTransform>(root_mtf);

        // Then go down to children and update their transforms.
        resolve_transforms_recursive(root_handle, root_mtf);
    }


    // TODO: This shouldn't be done here, but I need it for testing right now.
    for (const auto [entity, local_aabb, mtf] :
        registry.view<LocalAABB, MTransform>().each())
    {
        const entt::handle handle{ registry, entity };
        handle.emplace_or_replace<AABB>(local_aabb.transformed(mtf.model()));
    }
}





} // namespace josh::stages::precompute
