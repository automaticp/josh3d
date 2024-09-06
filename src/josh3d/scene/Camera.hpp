#pragma once
#include "ViewFrustum.hpp"
#include <glm/ext/matrix_transform.hpp>


namespace josh {
namespace detail {


/*
Implementation base for camera classes that contains the view frustum.

No meaning, just code reuse.
This had more stuff, and had more of a reason to exist. Not anymore.
*/
template<typename CRTP>
class CameraBase {
protected:
    ViewFrustumAsPlanes planes_local_frustum_;
    ViewFrustumAsQuads  quads_local_frustum_;

    CameraBase(
        const ViewFrustumAsPlanes& planes_local_frust,
        const ViewFrustumAsQuads&  quads_local_frust) noexcept
        : planes_local_frustum_{ planes_local_frust }
        , quads_local_frustum_ { quads_local_frust  }
    {}

    ~CameraBase() = default;

public:
    // Local view frustum in six-plane representation.
    auto view_frustum_as_planes() const noexcept
        -> const ViewFrustumAsPlanes&
    {
        return planes_local_frustum_;
    }

    // Local view frustum in two-quad representation.
    auto view_frustum_as_quads() const noexcept
        -> const ViewFrustumAsQuads&
    {
        return quads_local_frustum_;
    }

    CameraBase(const CameraBase&) = default;
    CameraBase(CameraBase&&) noexcept = default;
    CameraBase& operator=(const CameraBase&) = default;
    CameraBase& operator=(CameraBase&&) noexcept = default;
};


} // namespace detail




class PerspectiveCamera
    : public detail::CameraBase<PerspectiveCamera>
{
public:
    struct Params {
        float fovy_rad    { glm::radians(90.f) };
        float aspect_ratio{ 1.f                };
        float z_near      { 0.1f               };
        float z_far       { 1000.f             };
    };

    PerspectiveCamera(const Params& params) noexcept;

    auto get_params() const noexcept -> const Params&;
    void update_params(const Params& new_params) noexcept;

    auto projection_mat() const noexcept -> glm::mat4;

private:
    // We also store the same parameters that were used
    // to construct the frustum for future lookup.
    Params params_;
    using Base = detail::CameraBase<PerspectiveCamera>;
    void update_frustum_representations() noexcept;
};




/*
Simple camera with orthographic projection and an axially symmetric frustum.

No support for exotic assymetric or skewed frusuta, because that's just extra
complexity for now.
*/
class OrthographicCamera
    : public detail::CameraBase<OrthographicCamera>
{
public:
    struct Params {
        float width { 1.f    };
        float height{ 1.f    };
        float z_near{ 0.f    };
        float z_far { 1000.f };
    };

    OrthographicCamera(const Params& params) noexcept;

    auto get_params() const noexcept -> const Params&;
    void update_params(const Params& new_params) noexcept;

    auto projection_mat() const noexcept -> glm::mat4;

private:
    Params params_;
    using Base = detail::CameraBase<OrthographicCamera>;
    void update_frustum_representations() noexcept;
};




// Current typedef to be replaced later by a variant-like thing.
using Camera = PerspectiveCamera;


} // namespace josh
