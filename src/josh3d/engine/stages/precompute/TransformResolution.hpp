#pragma once
#include "RenderEngine.hpp"


namespace josh {


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
struct TransformResolution
{
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh
