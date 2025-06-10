#pragma once
#include "Common.hpp"
#include "Math.hpp"
#include "Scalars.hpp"


namespace josh {

struct Joint
{
    static constexpr u32 no_parent = -1;
    mat4 inv_bind   = {};
    u32  parent_idx = no_parent; // Index into the `joints` array of the skeleton.
};

/*
NOTE: This only defines the data layout of the skeleton.
Each skeleton itself as a resource must be referenced by the SkinnedMesh.

TODO: Encapsulate better.
*/
struct Skeleton
{
    static constexpr usize max_joints = 255;
    Vector<Joint> joints; // The joints of the skeleton stored in a pre-order. Up to `max_joints` total. Root is always first.
    String        name;   // Display name. Not unique.
};

} // namespace josh
