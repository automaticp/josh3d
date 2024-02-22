#pragma once
#include "RenderEngine.hpp"
#include "components/ChildMesh.hpp"
#include "components/Model.hpp"
#include "components/Transform.hpp"
#include <entt/entity/entity.hpp>


namespace josh::stages::precompute {

/*
This stage is responsible for computing the final world matrices for
all the meshes present in a scene.

The convention is that the `Transform` component represents a relative
transformation, possibly, in relation to a parent Model, while
the `MTransform` component is used to represent the final world-matrix
of anything transformable or a Mesh with the transforms chained from parent to child.
*/
class TransformResolution {
public:
    enum class Strategy {
        branch_on_children,
        models_then_branch_on_children,
        models_children_then_the_rest,
        top_down_then_the_rest
    } strategy{ Strategy::top_down_then_the_rest };

    void operator()(RenderEnginePrecomputeInterface& engine);

};




inline void TransformResolution::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    auto& registry = engine.registry();

    // There are several ways to iterate the registry for this:
    //
    //   1. Braindead iteration over each Transform and to compute MTransform for it
    //      irrespective of what kind of object it is, while branching for `ChildMesh`es
    //
    //   2. Iterate over each Model to compute its MTransforms first, then do a brainded
    //      pass like in the first option, excluding Models. This saves on recomputing
    //      the Model's MTransforms for each Mesh attached.
    //
    //   3. Compute MTransform for each model, then for each ChildMesh, then for everything
    //      else that is not either of the two.
    //
    //   4. Iterate top-down from each Model to its children updating their MTransforms,
    //      and then do a separate pass excluding all `ChildMesh`es
    //
    // The first one has a relatively cache-friendly memory access for components,
    // but can branch unpredictably, the second one is an improvement of the first,
    // with minimal losses, the third one only branches on filtering components,
    // but does many passes over the same memory, and the fourth one is less branchy
    // than the first two, but suffers from accessing components all-over-the-place.
    //
    // This is all of course completely useless without benchmarks in realistic scenarios,
    // and the performance could vary wildly just from how the entities are sorted.
    //
    // If the performance is eventually a problem, then I'll start thinking about this.

    switch (strategy) {
        case Strategy::branch_on_children: {

            for (auto [entity, transform] :
                registry.view<components::Transform>().each())
            {
                if (auto as_child = registry.try_get<components::ChildMesh>(entity)) {
                    // Redundantly recompute the parent MTransform since we have no way
                    // of knowing if it is up-to-date yet or not.
                    const auto& parent_mt = registry.get<components::Transform>(as_child->parent).mtransform();
                    registry.emplace_or_replace<components::MTransform>(entity, parent_mt * transform.mtransform());
                } else {
                    registry.emplace_or_replace<components::MTransform>(entity, transform.mtransform());
                }
            }


        } break;
        case Strategy::models_then_branch_on_children: {

            for (auto [entity, transform, model] :
                registry.view<components::Transform, components::Model>().each())
            {
                registry.emplace_or_replace<components::MTransform>(entity, transform.mtransform());
            }

            for (auto [entity, transform] :
                registry.view<components::Transform>(entt::exclude<components::Model>).each())
            {
                if (auto as_child = registry.try_get<components::ChildMesh>(entity)) {
                    const auto& parent_mt = registry.get<components::MTransform>(as_child->parent);
                    registry.emplace_or_replace<components::MTransform>(entity, parent_mt * transform.mtransform());
                } else {
                    registry.emplace_or_replace<components::MTransform>(entity, transform.mtransform());
                }
            }

        } break;
        case Strategy::models_children_then_the_rest: {

            for (auto [entity, transform, model] :
                registry.view<components::Transform, components::Model>().each())
            {
                registry.emplace_or_replace<components::MTransform>(entity, transform.mtransform());
            }

            for (auto [entity, transform, as_child] :
                // Models can't be ChildMeshes so no need to exclude them.
                registry.view<components::Transform, components::ChildMesh>().each())
            {
                const auto& parent_mt = registry.get<components::MTransform>(as_child.parent);
                registry.emplace_or_replace<components::MTransform>(entity, parent_mt * transform.mtransform());
            }

            for (auto [entity, transform] :
                registry.view<components::Transform>(
                    entt::exclude<components::Model, components::ChildMesh>).each())
            {
                registry.emplace_or_replace<components::MTransform>(entity, transform.mtransform());
            }

        } break;
        case Strategy::top_down_then_the_rest: {

            for (auto [entity, transform, model] :
                registry.view<components::Transform, components::Model>().each())
            {
                const auto& parent_mt =
                    registry.emplace_or_replace<components::MTransform>(entity, transform.mtransform());

                for (const auto& mesh_entity : model.meshes()) {
                    const auto& mesh_tf = registry.get<components::Transform>(mesh_entity);
                    registry.emplace_or_replace<components::MTransform>(
                        mesh_entity, parent_mt * mesh_tf.mtransform()
                    );
                }
            }

            for (auto [entity, transform] :
                registry.view<components::Transform>(
                    entt::exclude<components::Model, components::ChildMesh>).each())
            {
                registry.emplace_or_replace<components::MTransform>(entity, transform.mtransform());
            }

        } break;
    }


}




} // namespace josh::stages::precompute
