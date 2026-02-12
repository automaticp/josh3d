#pragma once
#include "Geometry.hpp"
#include "Math.hpp"


namespace josh {


/*
Simple bounding sphere that fully encloses the object.

As a component, this represents world-space bounding sphere.
*/
struct BoundingSphere : Sphere {
    auto transformed(const mat4& world_mat) const noexcept
        -> BoundingSphere;
};


/*
Bounding sphere in local space of the object.
*/
struct LocalBoundingSphere : BoundingSphere {};


} // namespace josh
