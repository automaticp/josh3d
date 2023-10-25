#pragma once
#include "CameraBase.hpp"
#include "Transform.hpp"
#include "ViewFrustum.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>



namespace josh {


struct PerspectiveCameraParams {
    float fovy_rad{ glm::radians(90.f) };
    float aspect_ratio{ 1.f };
    float z_near{ 0.1f };
    float z_far{ 1000.f };
};


class PerspectiveCamera
    : public detail::CameraBase<PerspectiveCamera>
{
private:
    // We also store the same parameters that were used
    // to construct the frustum for future lookup.
    PerspectiveCameraParams params_;
    using Base = detail::CameraBase<PerspectiveCamera>;

public:
    PerspectiveCamera(PerspectiveCameraParams params, const Transform& transform = {}) noexcept;

    auto get_params() const noexcept
        -> const PerspectiveCameraParams&;

    void update_params(const PerspectiveCameraParams& new_params) noexcept;

    auto projection_mat() const noexcept
        -> glm::mat4;

private:
    void update_frustum_representations() noexcept;

};




inline PerspectiveCamera::PerspectiveCamera(
    PerspectiveCameraParams params,
    const Transform& transform) noexcept
    : Base{
        ViewFrustumAsPlanes::make_local_perspective(
            params.fovy_rad, params.aspect_ratio,
            params.z_near, params.z_far
        ),
        ViewFrustumAsQuads::make_local_perspective(
            params.fovy_rad, params.aspect_ratio,
            params.z_near, params.z_far
        ),
        transform
    }
    , params_{ params }
{}



inline auto PerspectiveCamera::get_params() const noexcept
    -> const PerspectiveCameraParams&
{
    return params_;
}


inline void PerspectiveCamera::update_params(
    const PerspectiveCameraParams& params) noexcept
{
    params_ = params;
    update_frustum_representations();
}


inline auto PerspectiveCamera::projection_mat() const noexcept
    -> glm::mat4
{
    return glm::perspective(
        params_.fovy_rad, params_.aspect_ratio,
        params_.z_near, params_.z_far
    );
}


inline void PerspectiveCamera::update_frustum_representations() noexcept {
    planes_local_frustum_ = ViewFrustumAsPlanes::make_local_perspective(
        params_.fovy_rad, params_.aspect_ratio,
        params_.z_near, params_.z_far
    );
    quads_local_frustum_ = ViewFrustumAsQuads::make_local_perspective(
        params_.fovy_rad, params_.aspect_ratio,
        params_.z_near, params_.z_far
    );
}




} // namespace josh
