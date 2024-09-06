#include "Camera.hpp"


namespace josh {


PerspectiveCamera::PerspectiveCamera(const Params& params) noexcept
    : Base{
        ViewFrustumAsPlanes::make_local_perspective(
            params.fovy_rad, params.aspect_ratio,
            params.z_near,   params.z_far
        ),
        ViewFrustumAsQuads::make_local_perspective(
            params.fovy_rad, params.aspect_ratio,
            params.z_near,   params.z_far
        ),
    }
    , params_{ params }
{}


auto PerspectiveCamera::get_params() const noexcept
    -> const Params&
{
    return params_;
}


void PerspectiveCamera::update_params(const Params& params) noexcept {
    params_ = params;
    update_frustum_representations();
}


auto PerspectiveCamera::projection_mat() const noexcept
    -> glm::mat4
{
    return glm::perspective(
        params_.fovy_rad, params_.aspect_ratio,
        params_.z_near,   params_.z_far
    );
}


void PerspectiveCamera::update_frustum_representations() noexcept {
    planes_local_frustum_ = ViewFrustumAsPlanes::make_local_perspective(
        params_.fovy_rad, params_.aspect_ratio,
        params_.z_near,   params_.z_far
    );
    quads_local_frustum_ = ViewFrustumAsQuads::make_local_perspective(
        params_.fovy_rad, params_.aspect_ratio,
        params_.z_near,   params_.z_far
    );
}




OrthographicCamera::OrthographicCamera(const Params& params) noexcept
    : Base{
        ViewFrustumAsPlanes::make_local_orthographic(
            params.width,  params.height,
            params.z_near, params.z_far
        ),
        ViewFrustumAsQuads::make_local_z_symmetric(
            params.width,  params.height,
            params.width,  params.height,
            params.z_near, params.z_far
        ),
    }
    , params_{ params }
{}


auto OrthographicCamera::get_params() const noexcept
    -> const Params&
{
    return params_;
}


void OrthographicCamera::update_params(const Params& params) noexcept {
    params_ = params;
    update_frustum_representations();
}


auto OrthographicCamera::projection_mat() const noexcept
    -> glm::mat4
{
    const float hw = params_.width / 2.f;
    const float hh = params_.height / 2.f;
    return glm::ortho(-hw, hw, -hh, hh, params_.z_near, params_.z_far);
}


void OrthographicCamera::update_frustum_representations() noexcept {
    planes_local_frustum_ = ViewFrustumAsPlanes::make_local_orthographic(
        params_.width,  params_.height,
        params_.z_near, params_.z_far
    );
    quads_local_frustum_ = ViewFrustumAsQuads::make_local_z_symmetric(
        params_.width,  params_.height,
        params_.width,  params_.height,
        params_.z_near, params_.z_far
    );
}


} // namespace josh
