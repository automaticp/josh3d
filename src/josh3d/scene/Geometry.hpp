#pragma once
#include <glm/glm.hpp>
#include <array>


/*
Basic geometric primitives that don't deserve their own file.
*/
namespace josh {


// Convenience to express the standard basis in local space.
constexpr glm::vec3 X{ 1.f, 0.f, 0.f };
constexpr glm::vec3 Y{ 0.f, 1.f, 0.f };
constexpr glm::vec3 Z{ 0.f, 0.f, 1.f };




struct Quad {
    std::array<glm::vec3, 4> points{};

    auto transformed(const glm::mat4& world_mat) const noexcept
        -> Quad;
};


inline auto Quad::transformed(const glm::mat4& world_mat) const noexcept
    -> Quad
{
    return {
        world_mat * glm::vec4{ points[0], 1.f },
        world_mat * glm::vec4{ points[1], 1.f },
        world_mat * glm::vec4{ points[2], 1.f },
        world_mat * glm::vec4{ points[3], 1.f },
    };
}




struct Plane {
    // The normal vector representing the direction the plane is "facing".
    glm::vec3 normal{ 0.f, 0.f, -1.f };
    // The closest signed distance between the origin and the plane.
    // Can be negative to represent planes facing towards the origin.
    // The (closest_distance * normal) gives the position of
    // the closest to the origin point of the plane.
    float closest_distance{ 0.f };

    auto transformed(const glm::mat4& world_mat) const noexcept
        -> Plane;
};


// TODO: This probably should work with a change-of-basis matrix directly.
inline auto Plane::transformed(const glm::mat4& world_mat) const noexcept
    -> Plane
{
    const glm::mat3 L2W_mat3 = inverse(world_mat);
    const glm::vec3 position = world_mat[3];

    // WARN: I derived this from simplified 2D diagrams on paper.
    // I could easily be missing some important considerations that
    // appliy in 3D alone. Or I could be just dumb in some other way.
    // Either way, this may not work.
    //
    // WARN 2: I adopted this from a previous implemetation that used
    // a plain Transform instead of a full mat4. This may work even less now.

    const glm::vec3 new_normal = normalize(normal * L2W_mat3);

    // Reproject the parent (camera) position onto the new normal.
    // This should give us a separation distance between the new plane
    // and its local origin.
    const float new_closest_distance =
        closest_distance + glm::dot(new_normal, position);

    // For a simple transform a couple notable cases take place:
    //
    // - The near and far planes are just moved along the position axis
    //   by the position.length(). Normals and position lines are parallel:
    //   the dot product is 1 or -1 times position.length();
    //
    // - The side planes of the orthographic projection do not change their
    //   closest distance. The normal and position lines are perpendicular:
    //   the dot product is 0.

    return { new_normal, new_closest_distance };
}




struct Sphere {
    glm::vec3 position{};
    float     radius{ 1.f };

    // NOTE: No transformed() as non-orthogonal transformations
    // technically produce ellipsoids; transformed() would make
    // sense for the BoundingSphere, which resizes to fit instead.
};




} // namespace josh
