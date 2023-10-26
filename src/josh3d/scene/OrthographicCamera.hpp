#pragma once
#include "detail/CameraBase.hpp"
#include "Transform.hpp"
#include "ViewFrustum.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>


namespace josh {


struct OrthographicCameraParams {
    float width { 1.f };
    float height{ 1.f };
    float z_near{ 0.f };
    float z_far { 1000.f };
};

/*
Simple camera with orthographic projection and an axially symmetric frustum.

No support for exotic assymetric or skewed frusuta, because that's just extra
complexity for now.
*/
class OrthographicCamera
    : public detail::CameraBase<OrthographicCamera>
{
private:
    OrthographicCameraParams params_;
    using Base = detail::CameraBase<OrthographicCamera>;

public:
    OrthographicCamera(OrthographicCameraParams params,
        const Transform& transform = {}) noexcept;

    auto get_params() const noexcept
        -> const OrthographicCameraParams&;

    void update_params(const OrthographicCameraParams& new_params) noexcept;

    auto projection_mat() const noexcept
        -> glm::mat4;

private:
    void update_frustum_representations() noexcept;

};




inline OrthographicCamera::OrthographicCamera(
    OrthographicCameraParams params,
    const Transform& transform) noexcept
    : Base{
        ViewFrustumAsPlanes::make_local_orthographic(
            params.width, params.height,
            params.z_near, params.z_far
        ),
        ViewFrustumAsQuads::make_local_z_symmetric(
            params.width, params.height,
            params.width, params.height,
            params.z_near, params.z_far
        ),
        transform
    }
    , params_{ params }
{}


inline auto OrthographicCamera::get_params() const noexcept
    -> const OrthographicCameraParams&
{
    return params_;
}


inline void OrthographicCamera::update_params(
    const OrthographicCameraParams& params) noexcept
{
    params_ = params;
    update_frustum_representations();
}


inline auto OrthographicCamera::projection_mat() const noexcept
    -> glm::mat4
{
    const float hw = params_.width / 2.f;
    const float hh = params_.height / 2.f;
    return glm::ortho(-hw, hw, -hh, hh, params_.z_near, params_.z_far);
}


inline void OrthographicCamera::update_frustum_representations() noexcept {
    planes_local_frustum_ = ViewFrustumAsPlanes::make_local_orthographic(
        params_.width, params_.height,
        params_.z_near, params_.z_far
    );
    quads_local_frustum_ = ViewFrustumAsQuads::make_local_z_symmetric(
        params_.width, params_.height,
        params_.width, params_.height,
        params_.z_near, params_.z_far
    );
}



} // namespace josh
