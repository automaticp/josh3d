#pragma once
#include <cstdint>
#include "Math.hpp"


namespace josh {


struct Joint {
    static constexpr uint8_t no_parent = -1;
    mat4     inv_bind {};
    uint32_t parent_id{ no_parent }; // Index into the `joints` array of the skeleton.
};


/*
NOTE: This only defines the data layout of the skeleton.
Each skeleton itself as a resource must be referenced by the SkinnedMesh.

TODO: Encapsulate better.
*/
struct Skeleton {
    static constexpr size_t max_joints = 255;
    // The joints of the skeleton stored in a pre-order.
    // Up-to 255 joints total. Root is always first.
    std::vector<Joint> joints;
};


} // namespace josh
