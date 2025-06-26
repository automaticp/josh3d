#include "TransformResolution.hpp"
#include "Transform.hpp"
#include "SceneGraph.hpp"
#include "ECS.hpp"
#include "Tracy.hpp"


namespace josh {
namespace {

// Will get as deep as the deepest scene-graph tree. So should be pretty shallow.
void resolve_transforms_recursive(
    const Handle&     node_handle,
    const MTransform& node_mtf) noexcept
{
    if (has_children(node_handle))
    {
        for (const Handle child_handle : view_child_handles(node_handle))
        {
            const MTransform child_local_mtf = child_handle.get_or_emplace<Transform>().mtransform();
            const MTransform child_mtf       = node_mtf * child_local_mtf;
            child_handle.emplace_or_replace<MTransform>(child_mtf);

            resolve_transforms_recursive(child_handle, child_mtf);
        }
    }
}

} // namespace


void TransformResolution::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    ZSN("TransformResolution");
    auto& registry = engine.registry();

    // TODO: Two quirks, that are somewhat contradictory:
    // 1. This only operates on the root nodes that *have* the Transform already.
    // 2. All child nodes that *don't have* the Transform, get a default one emplaced.

    // TODO: The exclude<AsChild> is a negative filter that is
    // much more expensive to compute (O(N) scan) than a direct
    // tagged list of view<Root>().

    for (auto [root_entity, transform] :
        registry.view<Transform>(entt::exclude<AsChild>).each())
    {
        // Update root transform.
        const Handle     root_handle = { registry, root_entity  };
        const MTransform root_mtf    = transform.mtransform();
        root_handle.emplace_or_replace<MTransform>(root_mtf);

        // Then go down to children and update their transforms.
        resolve_transforms_recursive(root_handle, root_mtf);
    }
}





} // namespace josh
