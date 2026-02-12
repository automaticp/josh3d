#pragma once
#include "AABB.hpp"
#include "ViewFrustum.hpp"
#include <glm/glm.hpp>


/*
Intersection/collision tests for geometric primitives.

NOTE: Naming of free-functions here is a bit awkward.
Consider the first argument to be a `this` of a member function:

    is_fully_in_front_of(aabb, plane) -> aabb.is_fully_in_front_of(plane)

*/
namespace josh {


inline bool is_fully_in_front_of(
    const Sphere& sphere,
    const Plane&  plane) noexcept
{
    // Deliniates the enclosing volume of valid positions for the sphere center.
    const float closest_approach = plane.closest_distance + sphere.radius;

    // Projection of the sphere center onto the normal axis of the plane.
    const float normally_projected_distance = glm::dot(plane.normal, sphere.position);

    // This allows us to work with the distance along the normal axis of the plane.
    return normally_projected_distance > closest_approach;
}


inline bool is_fully_in_front_of(
    const AABB&  aabb,
    const Plane& plane) noexcept
{
    // Here quadrant of the normal is irrelevant, since the AABB extents
    // are symmetric wrt. to reflection around the midpoint.
    //
    // This, effectively, "selects" the closest vertex,
    // and computes the distance from it to the midpoint,
    // projected along the plane normal.
    //
    // Note that this selected vertex is really "closest" only
    // if it is in front of the plane. But the same vertex
    // is "selected" even if the AABB is intersecting, or behind the plane.
    const float projected_extent = dot((aabb.extents() / 2), abs(plane.normal));

    // Projected midpoint in world. Or closest distance from the plane
    // that is oriented by the same normal, but goes through the world origin.
    const float projected_midpoint = dot(aabb.midpoint(), plane.normal);

    // Along the plane normal, the following holds:
    //
    //      projected_midpoint =
    //          plane.closest_distance     +
    //          distance_to_closest_vertex +
    //          projected_extent
    //
    // where `distance_to_closest_vertex` is the closest distance
    // between the plane and the "closest" vertex.
    //
    // If that distance is positive, the "closest" vertex is
    // in front of the plane.
    const float distance_to_closest_vertex =
        (projected_midpoint - plane.closest_distance) - projected_extent;

    // TODO: There seems to be incorrect culling when above a plane-like object
    // and looking up ~40 degrees. Debug when frustum visualization is available.

    return distance_to_closest_vertex > 0.f;
}


inline bool is_fully_behind(
    const AABB&  aabb,
    const Plane& plane) noexcept
{
    return is_fully_in_front_of(aabb, { -plane.normal, -plane.closest_distance });
}


inline bool is_fully_outside_of(
    const AABB&          aabb,
    const FrustumPlanes& frustum) noexcept
{
    return
        is_fully_in_front_of(aabb, frustum.near()  ) ||
        is_fully_in_front_of(aabb, frustum.far()   ) ||
        is_fully_in_front_of(aabb, frustum.left()  ) ||
        is_fully_in_front_of(aabb, frustum.right() ) ||
        is_fully_in_front_of(aabb, frustum.bottom()) ||
        is_fully_in_front_of(aabb, frustum.top()   );
}


inline bool is_fully_inside_of(
    const AABB&          aabb,
    const FrustumPlanes& frustum) noexcept
{
    return
        is_fully_behind(aabb, frustum.near()  ) &&
        is_fully_behind(aabb, frustum.far()   ) &&
        is_fully_behind(aabb, frustum.left()  ) &&
        is_fully_behind(aabb, frustum.right() ) &&
        is_fully_behind(aabb, frustum.bottom()) &&
        is_fully_behind(aabb, frustum.top()   );
}


inline bool is_fully_outside_of(
    const Sphere&        sphere,
    const FrustumPlanes& frustum) noexcept
{
    return
        is_fully_in_front_of(sphere, frustum.near()  ) ||
        is_fully_in_front_of(sphere, frustum.far()   ) ||
        is_fully_in_front_of(sphere, frustum.left()  ) ||
        is_fully_in_front_of(sphere, frustum.right() ) ||
        is_fully_in_front_of(sphere, frustum.bottom()) ||
        is_fully_in_front_of(sphere, frustum.top()   );
}


} // namespace josh
