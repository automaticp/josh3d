#pragma once
#include "Transform.hpp"
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <cmath>



namespace josh {



struct Plane {
    // The normal vector representing the direction the plane is "facing".
    glm::vec3 normal{ 0.f, 0.f, -1.f };
    // The closest signed distance between the origin and the plane.
    // Can be negative to represent planes facing towards the origin.
    // The (closest_distance * normal) gives the position of
    // the closest to the origin point of the plane.
    float closest_distance{ 0.f };
};



namespace detail {


class ViewFrustumImplBase {
protected:
    Plane near_;
    Plane far_;
    Plane left_;
    Plane right_;
    Plane top_;
    Plane bottom_;

    // The FoV is not a property of the frustum,
    // but is inherent in the perspective projection.
    // The orthographic projection, for example, has no FoV,
    // but can still define a frustum as a rectangular box.

    ViewFrustumImplBase(
        Plane near, Plane far,
        Plane left, Plane right,
        Plane top,  Plane bottom)
        : near_{ near }, far_{ far }
        , left_{ left }, right_{ right }
        , top_{ top },   bottom_{ bottom }
    {}

    ~ViewFrustumImplBase() = default;

public:
    ViewFrustumImplBase(const ViewFrustumImplBase&) = default;
    ViewFrustumImplBase(ViewFrustumImplBase&&) = default;
    ViewFrustumImplBase& operator=(const ViewFrustumImplBase&) = default;
    ViewFrustumImplBase& operator=(ViewFrustumImplBase&&) = default;

    const Plane& near() const noexcept { return near_; }
    const Plane& far()    const noexcept { return far_;    }
    const Plane& left()   const noexcept { return left_;   }
    const Plane& right()  const noexcept { return right_;  }
    const Plane& top()    const noexcept { return top_;    }
    const Plane& bottom() const noexcept { return bottom_; }

};


} // namespace detail


/*
View frustum that is defined in the local space relative
to the object (usually, camera) it's attached to.
*/
class LocalViewFrustum final : public detail::ViewFrustumImplBase {
private:
    using detail::ViewFrustumImplBase::ViewFrustumImplBase;

public:
    // Constructs a frustum for a perspective projection
    // in the shape of a rectangular right pyramid frustum.
    static LocalViewFrustum from_perspective(
        float fovy_rad, float aspect_ratio,
        float z_near, float z_far) noexcept
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


    // Constructs a frustum for an orthographic projection
    // in the shape of a rectangular box.
    static LocalViewFrustum from_orthographic(
        float left_side, float right_side,
        float bottom_side, float top_side,
        float z_near, float z_far) noexcept
    {
        const Plane near { {  0.f,  0.f,  1.f }, z_near      };
        const Plane far  { {  0.f,  0.f, -1.f }, z_far       };
        const Plane right{ {  1.f,  0.f,  0.f }, right_side  };
        const Plane left { { -1.f,  0.f,  0.f }, left_side   };
        const Plane top  { {  0.f,  1.f,  0.f }, top_side    };
        const Plane btm  { {  0.f, -1.f,  0.f }, bottom_side };
        return { near, far, left, right, top, btm };
    }

};




/*
View frustum that exists in the world space.
*/
class ViewFrustum final : public detail::ViewFrustumImplBase {
private:
    using detail::ViewFrustumImplBase::ViewFrustumImplBase;

public:
    static ViewFrustum from_local_frustum(const LocalViewFrustum& local_frustum,
        const Transform& transform) noexcept
    {
        const glm::mat3 normal_model = transform.mtransform().normal_model();

        auto transform_plane = [&](const Plane& plane) -> Plane {
            // WARN: I derived this from simplified 2D diagrams on paper.
            // I could easily be missing some important considerations that
            // appliy in 3D alone. Or I could be just dumb in some other way.
            // Either way, this may not work.
            const glm::vec3 new_normal = normal_model * plane.normal;

            // Reproject the parent (camera) position onto the new normal.
            // This should give us a separation distance between the new plane
            // and its local origin.
            const float new_closest_distance =
                plane.closest_distance + glm::dot(new_normal, transform.position());
            // Do note that a couple edge cases take place:
            // - The near and far planes are just moved along the position axis
            //   by the position.length(). Normals and position lines are parallel:
            //   the dot product is 1 or -1 times position.length();
            // - The side planes of the orthographic projection do not change their
            //   closest distance. The normal and position lines are perpendicular:
            //   the dot product is 0.

            return { new_normal, new_closest_distance };
        };


        /*
        Plane near, Plane far,
        Plane left, Plane right,
        Plane top,  Plane bottom
        */
        return {
            transform_plane(local_frustum.near()), transform_plane(local_frustum.far()),
            transform_plane(local_frustum.left()), transform_plane(local_frustum.right()),
            transform_plane(local_frustum.top()),  transform_plane(local_frustum.bottom()),
        };
    }




};




} // namespace josh
