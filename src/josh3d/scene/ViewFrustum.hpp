#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <array>


namespace josh {


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



/*
Alternative description of the frustum based on a pair of near and
far quads, which is better suited for transformation and per-vertex operations.

Useful for tightly fitting the frustum in shadow mapping, for example.

It's much easier to keep two different representations around, than
to convert between them. They are not nicely-interconvertible, so to speak.
*/
class ViewFrustumAsQuads {
private:
    Quad near_;
    Quad far_;

    ViewFrustumAsQuads(const Quad& near, const Quad& far)
        : near_{ near }, far_{ far }
    {}

public:
    // Constructs a two-quad frustum in local space
    // with rectangular z-symmetric near and far planes.
    static auto make_local_z_symmetric(
        float near_width, float near_height,
        float far_width,  float far_height,
        float z_near,     float z_far) noexcept
            -> ViewFrustumAsQuads;

    // Consturcts a two-quad frustum in local space
    // for a perspective projection.
    static auto make_local_perspective(
        float fovy_rad, float aspect_ratio,
        float z_near,   float z_far) noexcept
            -> ViewFrustumAsQuads;

    // Returns a frustum transformed into world-space according to transform.
    auto transformed(const glm::mat4& world_mat) const noexcept
        -> ViewFrustumAsQuads;

    auto near() const noexcept -> const Quad& { return near_; }
    auto far()  const noexcept -> const Quad& { return far_;  }

};




inline auto ViewFrustumAsQuads::make_local_z_symmetric(
    float near_width, float near_height,
    float far_width,  float far_height,
    float z_near,     float z_far) noexcept
        -> ViewFrustumAsQuads
{
    const float nhw = near_width  / 2.f;
    const float nhh = near_height / 2.f;
    Quad near{
        glm::vec3{ -nhw,  nhh, z_near },
        glm::vec3{ -nhw, -nhh, z_near },
        glm::vec3{  nhw, -nhh, z_near },
        glm::vec3{  nhw,  nhh, z_near },
    };
    const float fhw = far_width  / 2.f;
    const float fhh = far_height / 2.f;
    Quad far{
        glm::vec3{ -fhw,  fhh, z_far },
        glm::vec3{ -fhw, -fhh, z_far },
        glm::vec3{  fhw, -fhh, z_far },
        glm::vec3{  fhw,  fhh, z_far },
    };
    return ViewFrustumAsQuads{ near, far };
}


inline auto ViewFrustumAsQuads::make_local_perspective(
    float fovy_rad, float aspect_ratio,
    float z_near,   float z_far) noexcept
        -> ViewFrustumAsQuads
{
    using std::tan, std::atan;

    const float vfov = fovy_rad;
    const float hfov = 2.f * atan((aspect_ratio) * tan(vfov / 2.f));

    const float height_far = 2.f * tan(vfov / 2.f) * z_far;
    const float width_far  = 2.f * tan(hfov / 2.f) * z_far;

    const float height_near = height_far * (z_near / z_far);
    const float width_near  = width_far  * (z_near / z_far);

    return ViewFrustumAsQuads::make_local_z_symmetric(
        width_near, height_near, width_far, height_far, z_near, z_far
    );
}


inline auto ViewFrustumAsQuads::transformed(const glm::mat4& world_mat) const noexcept
    -> ViewFrustumAsQuads
{
    return { near_.transformed(world_mat), far_.transformed(world_mat) };
}





struct Plane {
    // The normal vector representing the direction the plane is "facing".
    glm::vec3 normal{ 0.f, 0.f, -1.f };
    // The closest signed distance between the origin and the plane.
    // Can be negative to represent planes facing towards the origin.
    // The (closest_distance * normal) gives the position of
    // the closest to the origin point of the plane.
    float closest_distance{ 0.f };

    auto transformed(const glm::mat4& world_mat) const noexcept -> Plane;
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



/*
Representation of a view frustum that describes the frustum as 6 planes.

Better suited for frustum collision detection and culling.
*/
class ViewFrustumAsPlanes {
private:
    Plane near_;
    Plane far_;
    Plane left_;
    Plane right_;
    Plane top_;
    Plane bottom_;

    ViewFrustumAsPlanes(
        Plane near, Plane far,
        Plane left, Plane right,
        Plane top,  Plane bottom)
        : near_{ near }, far_   { far    }
        , left_{ left }, right_ { right  }
        , top_ { top  }, bottom_{ bottom }
    {}

public:
    // Constructs a local frustum for a perspective projection
    // in the shape of a rectangular right pyramid frustum.
    static auto make_local_perspective(
        float fovy_rad, float aspect_ratio,
        float z_near,   float z_far) noexcept
            -> ViewFrustumAsPlanes;

    // Constructs a local frustum for an orthographic projection
    // in the shape of a rectangular box.
    static auto make_local_orthographic(
        float left_side,   float right_side,
        float bottom_side, float top_side,
        float z_near,      float z_far) noexcept
            -> ViewFrustumAsPlanes;

    // Constructs a local frustum for an orthographic projection
    // in the shape of a view-axis symmetric rectangular box.
    static auto make_local_orthographic(
        float width,  float height,
        float z_near, float z_far) noexcept
            -> ViewFrustumAsPlanes;

    // Returns a frustum transformed into world-space according to transform.
    // WARN: Does not work for non-default scale, I think.
    // ViewFrustumAsPlanes to_world_space(const Transform& transform) const noexcept;

    // Returns a frustum transformed according to transform matrix.
    // TODO: This is an incredible description, indeed.
    auto transformed(const glm::mat4& world_mat) const noexcept
        -> ViewFrustumAsPlanes;

    auto near()   const noexcept -> const Plane& { return near_;   }
    auto far()    const noexcept -> const Plane& { return far_;    }
    auto left()   const noexcept -> const Plane& { return left_;   }
    auto right()  const noexcept -> const Plane& { return right_;  }
    auto top()    const noexcept -> const Plane& { return top_;    }
    auto bottom() const noexcept -> const Plane& { return bottom_; }
};




inline auto ViewFrustumAsPlanes::make_local_perspective(
    float fovy_rad, float aspect_ratio,
    float z_near,   float z_far) noexcept
        -> ViewFrustumAsPlanes
{
    // RH (X: right, Y: up, Z: back) coordinate system.

    Plane near{ { 0.f, 0.f,  1.f }, z_near };
    Plane far { { 0.f, 0.f, -1.f }, z_far  };

    using std::sin, std::cos, std::tan, std::atan;

    // aspect == w / h == tan(hfov/2) / tan(vfov/2)
    const float vfov = fovy_rad;
    const float hfov = 2.f * atan((aspect_ratio) * tan(vfov / 2.f));

    Plane right{ { cos(hfov / 2.f),   0.f,             sin(hfov / 2.f) } };
    Plane top  { { 0.f,               cos(vfov / 2.f), sin(vfov / 2.f) } };

    Plane left { { -right.normal.x,   0.f,             right.normal.z  } };
    Plane btm  { { 0.f,               -top.normal.y,   top.normal.z    } };

    return { near, far, left, right, top, btm };
}


inline auto ViewFrustumAsPlanes::make_local_orthographic(
    float left_side,   float right_side,
    float bottom_side, float top_side,
    float z_near,      float z_far) noexcept
        -> ViewFrustumAsPlanes
{
    const Plane near { {  0.f,  0.f,  1.f }, z_near      };
    const Plane far  { {  0.f,  0.f, -1.f }, z_far       };
    const Plane right{ {  1.f,  0.f,  0.f }, right_side  };
    const Plane left { { -1.f,  0.f,  0.f }, left_side   };
    const Plane top  { {  0.f,  1.f,  0.f }, top_side    };
    const Plane btm  { {  0.f, -1.f,  0.f }, bottom_side };
    return { near, far, left, right, top, btm };
}


inline auto ViewFrustumAsPlanes::make_local_orthographic(
    float width,  float height,
    float z_near, float z_far) noexcept
        -> ViewFrustumAsPlanes
{
    const float hw = width  / 2.f;
    const float hh = height / 2.f;
    return make_local_orthographic(-hw, hw, -hh, hh, z_near, z_far);
}


inline auto ViewFrustumAsPlanes::transformed(const glm::mat4& world_mat) const noexcept
    -> ViewFrustumAsPlanes
{
    return {
        near_.transformed(world_mat), far_   .transformed(world_mat),
        left_.transformed(world_mat), right_ .transformed(world_mat),
        top_ .transformed(world_mat), bottom_.transformed(world_mat),
    };
}




} // namespace josh
