#pragma once
#include "Geometry.hpp"
#include <glm/glm.hpp>
#include <cmath>


namespace josh {


/*
Alternative description of the frustum based on a pair of near and
far quads, which is better suited for transformation and per-vertex operations.

Useful for tightly fitting the frustum in shadow mapping, for example.

It's much easier to keep two different representations around, than
to convert between them. They are not nicely-interconvertible, so to speak.
*/
class FrustumQuads {
private:
    Quad near_;
    Quad far_;

    FrustumQuads(const Quad& near, const Quad& far)
        : near_{ near }, far_{ far }
    {}

public:
    // Constructs a two-quad frustum in local space
    // with rectangular z-symmetric near and far planes.
    static auto make_local_z_symmetric(
        float near_width, float near_height,
        float far_width,  float far_height,
        float z_near,     float z_far) noexcept
            -> FrustumQuads;

    // Consturcts a two-quad frustum in local space
    // for a perspective projection.
    static auto make_local_perspective(
        float fovy_rad, float aspect_ratio,
        float z_near,   float z_far) noexcept
            -> FrustumQuads;

    // Returns a frustum transformed into world-space according to transform.
    auto transformed(const glm::mat4& world_mat) const noexcept
        -> FrustumQuads;

    auto near() const noexcept -> const Quad& { return near_; }
    auto far()  const noexcept -> const Quad& { return far_;  }

};


/*
Representation of a view frustum that describes the frustum as 6 planes.

Better suited for frustum collision detection and culling.

By convention, each plane is facing *outwards* from the frustum volume.
*/
class FrustumPlanes {
private:
    Plane near_;
    Plane far_;
    Plane left_;
    Plane right_;
    Plane top_;
    Plane bottom_;

    FrustumPlanes(
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
            -> FrustumPlanes;

    // Constructs a local frustum for an orthographic projection
    // in the shape of a rectangular box.
    static auto make_local_orthographic(
        float left_side,   float right_side,
        float bottom_side, float top_side,
        float z_near,      float z_far) noexcept
            -> FrustumPlanes;

    // Constructs a local frustum for an orthographic projection
    // in the shape of a view-axis symmetric rectangular box.
    static auto make_local_orthographic(
        float width,  float height,
        float z_near, float z_far) noexcept
            -> FrustumPlanes;

    // Returns a frustum transformed into world-space according to transform.
    // WARN: Does not work for non-default scale, I think.
    // ViewFrustumAsPlanes to_world_space(const Transform& transform) const noexcept;

    // Returns a frustum transformed according to transform matrix.
    // TODO: This is an incredible description, indeed.
    auto transformed(const glm::mat4& world_mat) const noexcept
        -> FrustumPlanes;

    auto near()   const noexcept -> const Plane& { return near_;   }
    auto far()    const noexcept -> const Plane& { return far_;    }
    auto left()   const noexcept -> const Plane& { return left_;   }
    auto right()  const noexcept -> const Plane& { return right_;  }
    auto top()    const noexcept -> const Plane& { return top_;    }
    auto bottom() const noexcept -> const Plane& { return bottom_; }
};


} // namespace josh
