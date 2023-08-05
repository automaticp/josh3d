#pragma once
#include "Basis.hpp"
#include "GlobalsUtil.hpp"
#include "Transform.hpp"
#include "ViewFrustum.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>



namespace josh {


struct PerspectiveCameraParams {
    float fov_rad;
    float aspect_ratio;
    float z_near;
    float z_far;

    PerspectiveCameraParams(
        float fov_rad, float aspect_ratio,
        float z_near, float z_far)
        : fov_rad{ fov_rad }, aspect_ratio{ aspect_ratio }
        , z_near{ z_near }, z_far{ z_far }
    {}
};


class PerspectiveCamera {
private:
    LocalViewFrustum frustum_;
    // We also store the same parameters that were used
    // to construct the frustum for future lookup.
    PerspectiveCameraParams params_;

public:
    // The camera isn't an entity as of now, so we store
    // the transform inside, available as a public member.
    //
    // The scale is so far ignored inside this class,
    // but it will affect the constructible model matrix.
    // If you DO set the scale to something other than (1, 1, 1),
    // then there's a chance things become unexpectedly broken
    // in other places. Good luck!
    Transform transform;

    PerspectiveCamera(PerspectiveCameraParams params) noexcept;
    PerspectiveCamera(const Transform& transform, PerspectiveCameraParams params) noexcept;

    // World-space ViewFrustum.
    auto get_view_frustum() const noexcept
        -> ViewFrustum;

    // You probably want the world-space ViewFrustum instead.
    auto get_local_view_frustum() const noexcept
        -> const LocalViewFrustum&;

    auto get_params() const noexcept
        -> const PerspectiveCameraParams&;

    void update_params(const PerspectiveCameraParams& new_params) noexcept;

    // Constructs local camera basis from the current transform:
    // x - right, y - up, z - back.
    auto get_local_basis() const noexcept
        -> OrthonormalBasis3D;

    auto view_mat() const noexcept
        -> glm::mat4;

    auto projection_mat() const noexcept
        -> glm::mat4;

};




inline PerspectiveCamera::PerspectiveCamera(
    PerspectiveCameraParams params) noexcept
    : PerspectiveCamera{
        Transform{
            glm::vec3{ 0.f },
            glm::quat{ 0.f, 0.f, -1.f, 0.f },
            glm::vec3{ 1.f }
        },
        params
    }
{}


inline PerspectiveCamera::PerspectiveCamera(
    const Transform& transform,
    PerspectiveCameraParams params) noexcept
    : frustum_{
        LocalViewFrustum::from_perspective(
            params.fov_rad, params.aspect_ratio,
            params.z_near, params.z_far)
    }
    , params_{ params }
    , transform{ transform }
{}




inline auto PerspectiveCamera::get_view_frustum() const noexcept
    -> ViewFrustum
{
    return ViewFrustum::from_local_frustum(
        frustum_,
        Transform{
            transform.position(),
            transform.rotation(),
            glm::vec3{ 1.f } // Strip away the scale.
        }
    );
}


inline auto PerspectiveCamera::get_local_view_frustum() const noexcept
    -> const LocalViewFrustum&
{
    return frustum_;
}


inline auto PerspectiveCamera::get_params() const noexcept
    -> const PerspectiveCameraParams&
{
    return params_;
}


inline void PerspectiveCamera::update_params(
    const PerspectiveCameraParams& params) noexcept
{
    params_ = params;
    frustum_ = LocalViewFrustum::from_perspective(
        params_.fov_rad, params_.aspect_ratio,
        params_.z_near, params_.z_far
    );
}


inline auto PerspectiveCamera::get_local_basis() const noexcept
    -> OrthonormalBasis3D
{
    OrthonormalBasis3D basis{ globals::basis };
    basis.rotate(transform.rotation());
    return basis;
}


inline auto PerspectiveCamera::view_mat() const noexcept
    -> glm::mat4
{
    auto local_basis = get_local_basis();
    return glm::lookAt(
        transform.position(),
        transform.position() - local_basis.z(),
        local_basis.y()
    );
}


inline auto PerspectiveCamera::projection_mat() const noexcept
    -> glm::mat4
{
    return glm::perspective(
        params_.fov_rad, params_.aspect_ratio,
        params_.z_near, params_.z_far
    );
}


} // namespace josh
