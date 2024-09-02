#pragma once
#include "RenderEngine.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>


namespace josh::stages::precompute {


/*
This stage is responsible for computing the final world matrices for
all the meshes present in a scene.

The convention is that the `Transform` component represents a relative
transformation, possibly, in relation to a parent node, while
the `MTransform` component is used to represent the final world-matrix
with the transforms chained from the root of the scene-graph.

TODO: I'm not really sure if this should be a precompute stage, or
existence of MTransforms is just part of the contract in displaying the entities.
*/
class TransformResolution {
public:
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh::stages::precompute
