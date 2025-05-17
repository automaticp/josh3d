#pragma once
#include "Math.hpp"


namespace josh {


/*
AABB in arbitrary space.

As a component, it represents world-space AABB.
*/
struct AABB {
    vec3 lbb; // Left-Bottom-Back
    vec3 rtf; // Right-Top-Front

    auto extents()  const noexcept -> vec3 { return rtf - lbb;          }
    auto midpoint() const noexcept -> vec3 { return (rtf + lbb) * 0.5f; }

    auto transformed(const mat4& world_mat) const noexcept -> AABB;
};


/*
AABB in local space of the object.
*/
struct LocalAABB : AABB {};


} // namespace josh
