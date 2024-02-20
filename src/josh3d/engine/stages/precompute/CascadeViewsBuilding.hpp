#pragma once
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ViewFrustum.hpp"
#include "SharedStorage.hpp"
#include "PerspectiveCamera.hpp"
#include "Transform.hpp"
#include "Basis.hpp"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/orthonormalize.hpp>
#include <vector>


namespace josh {


struct CascadeView {
    ViewFrustumAsPlanes frustum;
    glm::mat4 view;
    glm::mat4 projection;
    float     z_split;
};


/*
Some info about view frustums that were constructed from camera.
*/
struct CascadeViews {
    std::vector<CascadeView> cascades;
    // TODO:
    // ViewFrustumAsPlanes bounding_frustum;
};


} // namespace josh




namespace josh::stages::precompute {


class CascadeViewsBuilding {
private:
    SharedStorage<CascadeViews> output_;

public:
    // This controls how many cascades will be present in a cascaded map.
    // On change of this value, CascadeViewsBuilder will resize the
    // output next time the cascades are rebuilt.
    //
    // Exceeding max_cascades of the CascadedShadowMappingStage
    // might yield surprising results.
    size_t num_cascades_to_build{ 5 };

    SharedStorageView<CascadeViews> share_output_view() const noexcept {
        return output_.share_view();
    }

    const CascadeViews& view_output() const noexcept { return *output_; }

    void operator()(RenderEnginePrecomputeInterface& engine);

private:
    void build_from_camera(const PerspectiveCamera& cam, const glm::vec3& light_dir) noexcept;
};




inline void CascadeViewsBuilding::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    build_from_camera(
        engine.camera(),
        engine.registry().view<light::Directional>().storage().begin()->direction
    );
}




inline void CascadeViewsBuilding::build_from_camera(
    const PerspectiveCamera& cam, const glm::vec3& light_dir) noexcept
{

    // WARN: This is still heavily WIP.

    const ViewFrustumAsPlanes planes_frust = cam.get_frustum_as_planes();
    const ViewFrustumAsQuads  quads_frust  = cam.get_frustum_as_quads();
    const OrthonormalBasis3D  cam_basis    = cam.get_local_basis();


    const float largest_observable_length = // OR IS IT?
        glm::length(cam.get_local_frustum_as_quads().far().points[0]);
    // There's still clipping for the largest cascade.

    const float z_near{ 0.f };
    const float z_far { 2 * largest_observable_length };

    // Similar to cam_offset in simple shadow mapping.
    const float cam_offset{ (z_far - z_near) / 2.f };

    // TODO: What's a good upvector?
    // Global basis upvector is a good choice because it doesn't
    // rotate the cascade with the frustum, reducing shimmer.
    const glm::vec3 shadow_cam_upvector{
        globals::basis.y()
        // glm::orthonormalize(-cam_basis.z(), globals::basis.y())
    };

    // Techincally, there's no position, but this marks the Z = 0 point
    // for each shadow camera in world space. This, together with the quatLookAt
    // is used to get the proper Transform for the construction of the
    // world-space ViewFrustum for each shadow camera.
    const glm::vec3 shadow_cam_position{
        cam.transform.position() - (cam_offset * light_dir)
    };

    const glm::mat4 shadow_look_at = glm::lookAt(
        shadow_cam_position,
        cam.transform.position(),
        shadow_cam_upvector
    );

    const glm::quat shadow_look_at_quat = glm::quatLookAt(
        light_dir, shadow_cam_upvector
    );

    // The view space is shared across all cascades.
    // Each cascade "looks at" the camera origin from the same Z = 0 point.
    // The only difference is in the horizontal/vertical projection boundaries.
    // TODO: This *might* be worth changing to allow different Z = 0 points per cascade.
    const Transform shadow_view_transform{
        shadow_cam_position, shadow_look_at_quat, glm::vec3{ 1.f }
    };

    const auto cam_frust_in_shadow_view = quads_frust.transformed(shadow_look_at);

    const auto& near = cam_frust_in_shadow_view.near();
    const auto& far  = cam_frust_in_shadow_view.far();


    // We find the min-max fits for the frustum in the camera view space.
    // We ignore the Z axis, as the depth of the projection is governed
    // by the depth of the camera frustum in it's local space, not in the
    // shadow's view space.
    // That way, whenever an object is visible in the camera, it will
    // cast shadows, irrespective of the rotation of the frustum.
    //
    // Well, actually, you probably want the distance to one of the corners
    // of the far plane, to account for most extreme cases as well.
    // FIXME: This might be the cause of the far side shadow clipping.
    constexpr float inf{ std::numeric_limits<float>::infinity() };
    glm::vec2 min{  inf,  inf };
    glm::vec2 max{ -inf, -inf };
    {
        auto minmax_update_from_corner = [&](const glm::vec3& corner) {
            min.x = std::min(min.x, corner.x);
            min.y = std::min(min.y, corner.y);
            max.x = std::max(max.x, corner.x);
            max.y = std::max(max.y, corner.y);
        };

        for (const glm::vec3& corner : cam_frust_in_shadow_view.near().points) {
            minmax_update_from_corner(corner);
        }
        for (const glm::vec3& corner : cam_frust_in_shadow_view.far().points) {
            minmax_update_from_corner(corner);
        }
    }

    // The size of the largest cascade is taken from the largest diagonal
    // of the camera frustum, so that it is independant of the frustum orientation.
    const float max_scale = glm::max(
        glm::distance(far.points[0], far.points[2]),
        glm::distance(far.points[0], near.points[2])
    );


    // This is taken from:
    //     F. Zhang et. al.
    //     "Parallel-Split Shadow Maps for Large-scale Virtual Environments"
    //     DOI: 10.1145/1128923.1128975
    //
    // However, this is applied to a simple [0, max_scale] space.
    // The question of what space to apply the split logic in is still an open one.
    auto split_fun_log = [&](size_t split_id) {
        return glm::pow(max_scale, float(split_id + 1) / float(num_cascades_to_build));
    };

    auto split_fun_uniform = [&](size_t split_id) {
        return max_scale * float(split_id + 1) / float(num_cascades_to_build);
    };

    // TODO: Might be weightable and also have a bias parameter.
    auto split_fun_practical = [&](size_t split_id) {
        return (split_fun_log(split_id) + split_fun_uniform(split_id)) / 2.f;
    };

    output_->cascades.clear();
    for (size_t i{ 0 }; i < num_cascades_to_build; ++i) {

        float split_side = split_fun_practical(i);

        float b = -split_side / 2.f;
        float t = +split_side / 2.f;
        float l = -split_side / 2.f;
        float r = +split_side / 2.f;


        const glm::mat4 shadow_projection =
            glm::ortho(l, r, b, t, z_near, z_far);

        output_->cascades.emplace_back(CascadeView{
            .frustum =
                ViewFrustumAsPlanes::make_local_orthographic(l, r, b, t, z_near, z_far)
                    .to_world_space(shadow_view_transform),
            .view       = shadow_look_at,
            .projection = shadow_projection,
            .z_split    = 0.f // FIXME: This
        });

    }

}




} // namespace josh::stages::precompute


