#pragma once
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "Active.hpp"
#include "ViewFrustum.hpp"
#include "SharedStorage.hpp"
#include "Camera.hpp"
#include "Transform.hpp"
#include "tags/ShadowCasting.hpp"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/round.hpp>
#include <glm/gtx/orthonormalize.hpp>
#include <vector>


namespace josh {


struct CascadeView {
    ViewFrustumAsPlanes frustum;
    glm::mat4 view;
    glm::mat4 proj;
    float     z_split;
};


/*
Some info about view frustums that were constructed from camera.
*/
struct CascadeViews {
    std::vector<CascadeView> cascades;
    Size2I resolution{ 2048, 2048 };
    // TODO:
    // ViewFrustumAsPlanes bounding_frustum;
};


} // namespace josh




namespace josh::stages::precompute {


class CSMSetup {
private:
    SharedStorage<CascadeViews> output_;

public:
    // This controls how many cascades will be present in a cascaded map.
    // On change of this value, CascadeViewsBuilder will resize the
    // output next time the cascades are rebuilt.
    //
    // Exceeding max_cascades of the CascadedShadowMappingStage
    // might yield surprising results.
    size_t num_cascades_to_build{ 4 };

    // Per-cascade resolution
    Size2I resolution{ 2048, 2048 };

    float split_log_weight{ 0.95f };
    float split_bias{ 0.f };

    SharedStorageView<CascadeViews> share_output_view() const noexcept {
        return output_.share_view();
    }

    const CascadeViews& view_output() const noexcept { return *output_; }

    void operator()(RenderEnginePrecomputeInterface& engine);

private:
    void build_from_camera(
        const glm::vec3&           cam_position,
        const ViewFrustumAsPlanes& frustum_as_planes,
        const ViewFrustumAsQuads&  frustum_as_quads,
        const glm::vec3&           light_dir) noexcept;
};




inline void CSMSetup::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    if (const auto dlight =
        get_active<DirectionalLight, Transform, ShadowCasting>(engine.registry()))
    {
        // TODO: Should be decompose_orientation() from MTransform.
        const glm::vec3 light_dir =
            dlight.get<Transform>().orientation() * glm::vec3{ 0.f, 0.f, -1.f };

        if (const auto camera = get_active<Camera, MTransform>(engine.registry())) {
            const MTransform& mtf       = camera.get<MTransform>();
            const glm::mat4&  world_mat = mtf.model();
            const Camera&     cam       = camera.get<Camera>();
            build_from_camera(
                mtf.decompose_position(),
                cam.view_frustum_as_planes().transformed(world_mat),
                cam.view_frustum_as_quads() .transformed(world_mat),
                light_dir
            );
        }

    }
}




inline void CSMSetup::build_from_camera(
    const glm::vec3&           cam_position,
    const ViewFrustumAsPlanes& frustum_as_planes, // In world-space.
    const ViewFrustumAsQuads&  frustum_as_quads,  // In world-space.
    const glm::vec3&           light_dir) noexcept
{
    using glm::vec2, glm::vec3, glm::mat3, glm::mat4, glm::quat;

    // WARN: This is still heavily WIP.

    // TODO: There's still clipping for the largest cascade.
    const float largest_observable_length = // OR IS IT?
        glm::length(frustum_as_quads.far().points[0] - cam_position);

    const float z_near{ 0.f };
    const float z_far { 2 * largest_observable_length };

    // Similar to cam_offset in simple shadow mapping.
    const float cam_offset{ (z_far - z_near) / 2.f };

    // TODO: What's a good upvector?
    // Global basis upvector is a good choice because it doesn't
    // rotate the cascade with the frustum, reducing shimmer.
    const vec3 shadow_cam_upvector = glm::vec3{ 0.f, 1.f, 0.f };
        // glm::orthonormalize(-cam_basis.z(), globals::basis.y())

    // Techincally, there's no position, but this marks the Z = 0 point
    // for each shadow camera in world space. This, together with the quatLookAt
    // is used to get the proper Transform for the construction of the
    // world-space ViewFrustum for each shadow camera.
    const vec3 shadow_cam_position = cam_position - (cam_offset * light_dir);

    const mat4 shadow_look_at      = glm::lookAt(shadow_cam_position, cam_position, shadow_cam_upvector);
    const quat shadow_look_at_quat = glm::quatLookAt(light_dir, shadow_cam_upvector);

    // The view space is shared across all cascades.
    // Each cascade "looks at" the camera origin from the same Z = 0 point.
    // The only difference is in the horizontal/vertical projection boundaries.
    // TODO: This *might* be worth changing to allow different Z = 0 points per cascade.
    const Transform shadow_view_transform{ shadow_cam_position, shadow_look_at_quat, glm::vec3{ 1.f } };

    // Shadow look_at is a view matrix of shadowcam-space, which is a shadow->world CoB.
    // We use it to transform the contravariant frustum points from world to shadow-space.
    const auto cam_frust_in_shadow_view = frustum_as_quads.transformed(shadow_look_at);

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
    vec2 min{  inf,  inf };
    vec2 max{ -inf, -inf };
    {
        auto minmax_update_from_corner = [&](const vec3& corner) {
            min.x = std::min(min.x, corner.x);
            min.y = std::min(min.y, corner.y);
            max.x = std::max(max.x, corner.x);
            max.y = std::max(max.y, corner.y);
        };

        for (const vec3& corner : cam_frust_in_shadow_view.near().points) {
            minmax_update_from_corner(corner);
        }
        for (const vec3& corner : cam_frust_in_shadow_view.far().points) {
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

    auto split_fun_practical = [&](size_t split_id, float log_weight, float bias) {
        log_weight = glm::clamp(log_weight, 0.f, 1.f);
        return    (log_weight) * split_fun_log(split_id) +
            (1.f - log_weight) * split_fun_uniform(split_id) + bias;
    };

    output_->cascades.clear();
    for (size_t i{ 0 }; i < num_cascades_to_build; ++i) {

        float split_side = split_fun_practical(i, split_log_weight, split_bias);

        float l = -split_side / 2.f;
        float r = +split_side / 2.f;
        float b = -split_side / 2.f;
        float t = +split_side / 2.f;

        auto snap_to_grid = [&](float& l, float& r, float& b, float& t) {
            // FIXME: This, as any other world-space computation will completely
            // break down when far away from the origin.
            // The addition and subtraction of `center.x` will obliterate
            // the contribution of a small pixel-scale correction fairly quickly too.

            // This is a position of the shadowcam in space that's oriented like
            // the shadow view but centered on the world origin.
            const vec3 center = mat3(shadow_look_at) * shadow_cam_position;

            // Size of a single shadowmap pixel in shadowmap view-space.
            Extent2F px_scale{
                split_side / float(resolution.width),
                split_side / float(resolution.height)
            };

            // Frustum bounds in world-space oriented as shadowcam.
            float bv = b + center.y;
            float tv = t + center.y;
            float lv = l + center.x;
            float rv = r + center.x;

            // glm::floorMultiple uses subtraction in lower precision space,
            // instead of divide-floor-multiply which floors in higher precision.
            // Kinda suboptimal, oh well, this is broken anyway.
            l = glm::floorMultiple(lv, px_scale.width)  - center.x;
            r = glm::floorMultiple(rv, px_scale.width)  - center.x;
            b = glm::floorMultiple(bv, px_scale.height) - center.y;
            t = glm::floorMultiple(tv, px_scale.height) - center.y;

        };

        snap_to_grid(l, r, b, t);

        const mat4 shadow_proj = glm::ortho(l, r, b, t, z_near, z_far);

        output_->cascades.emplace_back(CascadeView{
            .frustum =
                ViewFrustumAsPlanes::make_local_orthographic(l, r, b, t, z_near, z_far)
                    .transformed(shadow_view_transform.mtransform().model()),
            .view    = shadow_look_at,
            .proj    = shadow_proj,
            .z_split = 0.f // FIXME: This
        });

    }
    output_->resolution = resolution;
}




} // namespace josh::stages::precompute


