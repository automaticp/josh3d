#pragma once
#include "ViewFrustum.hpp"
#include "Basis.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace josh::detail {


/*
Implementation base for camera classes that contains the
view frustum and transforms.

No meaning, just code reuse.
*/
template<typename CRTP>
class CameraBase {
protected:
    ViewFrustumAsPlanes planes_local_frustum_;
    ViewFrustumAsQuads  quads_local_frustum_;

    CameraBase(
        const ViewFrustumAsPlanes& planes_local_frust,
        const ViewFrustumAsQuads& quads_local_frust,
        const Transform& transform) noexcept
        : planes_local_frustum_{ planes_local_frust }
        , quads_local_frustum_{ quads_local_frust }
        , transform{ transform }
    {}

    ~CameraBase() = default;

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

    // World-space view frustum in six-plane representation.
    auto get_frustum_as_planes() const noexcept
        -> ViewFrustumAsPlanes
    {
        return planes_local_frustum_.to_world_space(
            Transform{ transform.position(), transform.rotation(), glm::vec3{ 1.f } }
        );
    }

    // World-space view frustum in two-quad representation.
    auto get_frustum_as_quads() const noexcept
        -> ViewFrustumAsQuads
    {
        return quads_local_frustum_.to_world_space(
            Transform{ transform.position(), transform.rotation(), glm::vec3{ 1.f } }
        );
    }

    // Local view frustum in six-plane representation.
    auto get_local_frustum_as_planes() const noexcept
        -> const ViewFrustumAsPlanes&
    {
        return planes_local_frustum_;
    }

    // Local view frustum in two-quad representation.
    auto get_local_frustum_as_quads() const noexcept
        -> const ViewFrustumAsQuads&
    {
        return quads_local_frustum_;
    }

    // Constructs local camera basis from the current transform:
    // x - right, y - up, z - back.
    auto get_local_basis() const noexcept
        -> OrthonormalBasis3D
    {
        OrthonormalBasis3D basis{ globals::basis };
        basis.rotate(transform.rotation());
        return basis;
    }

    // Constructs the view matrix for this camera.
    auto view_mat() const noexcept
        -> glm::mat4
    {
        auto local_basis = get_local_basis();
        return glm::lookAt(
            transform.position(),
            transform.position() - local_basis.z(),
            globals::basis.y()
        );
    }


    CameraBase(const CameraBase&) = default;
    CameraBase(CameraBase&&) noexcept = default;
    CameraBase& operator=(const CameraBase&) = default;
    CameraBase& operator=(CameraBase&&) noexcept = default;
};







} // namespace josh::detail
