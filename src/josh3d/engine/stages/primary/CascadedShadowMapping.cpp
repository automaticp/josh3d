#include "CascadedShadowMapping.hpp"
#include "Active.hpp"
#include "Camera.hpp"
#include "Components.hpp"
#include "GLAPIBinding.hpp"
#include "GLProgram.hpp"
#include "GLUniformTraits.hpp"
#include "GeometryCollision.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ViewFrustum.hpp"
#include "tags/AlphaTested.hpp"
#include "Materials.hpp"
#include "Transform.hpp"
#include "Mesh.hpp"
#include "UniformTraits.hpp"
#include "tags/ShadowCasting.hpp"
#include <algorithm>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glbinding-aux/Meta.h>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/functions.h>
#include <ranges>
#include <span>




namespace josh::stages::primary {


CascadedShadowMapping::CascadedShadowMapping(
    size_t        num_desired_cascades,
    const Size2I& resolution
)
    : resolution   { resolution }
    , num_cascades_{ allowed_num_cascades(num_desired_cascades) }
    , cascades_{ Cascades{
        .maps      { resolution, GLsizei(num_cascades_), { InternalFormat::DepthComponent32F } },
        .resolution{ resolution },
        .views     {}
    } }
{}


CascadedShadowMapping::CascadedShadowMapping()
    : CascadedShadowMapping(4, { 2048, 2048 })
{}


auto CascadedShadowMapping::allowed_num_cascades(size_t desired_num) const noexcept
    -> size_t
{
    return std::clamp(desired_num, size_t(1), max_cascades());
}


auto CascadedShadowMapping::num_cascades() const noexcept
    -> size_t
{
    return num_cascades_;
}


auto CascadedShadowMapping::max_cascades() const noexcept
    -> size_t
{
    // TODO: Query API limits.
    return 7; // Chosen by a fair rice doll.
}


void CascadedShadowMapping::set_num_cascades(size_t desired_num) noexcept {
    num_cascades_ = allowed_num_cascades(desired_num);
}




namespace {


using CascadeView = CascadedShadowMapping::CascadeView;


void fit_cascade_views_to_camera(
    std::vector<CascadeView>& out_views,
    size_t                    num_cascades,
    const Size2I&             resolution,
    const glm::vec3&          cam_position,
    const ViewFrustumAsQuads& frustum_world,
    const glm::vec3&          light_dir,
    float                     split_log_weight,
    float                     split_bias,
    bool                      pad_inner_cascades,
    float                     padding_tx)
{
    using glm::vec2, glm::vec3, glm::mat3, glm::mat4, glm::quat;

    // WARN: This is still heavily WIP.

    // TODO: There's still clipping for the largest cascade.
    const float largest_observable_length = // OR IS IT?
        glm::length(frustum_world.far().points[0] - cam_position);

    const float z_near{ 0.f };
    const float z_far { 2 * largest_observable_length };

    // Similar to cam_offset in simple shadow mapping.
    const float cam_offset{ (z_far - z_near) / 2.f };

    // Global basis upvector is a good choice because it doesn't
    // rotate the cascade with the frustum, reducing shimmer.
    const vec3 shadow_cam_upvector = Y;

    // Techincally, there's no position, but this marks the Z = 0 point
    // for each shadow camera in world space.
    const vec3 shadow_cam_position = cam_position - (cam_offset * light_dir);

    // The view space is shared across all cascades.
    // Each cascade "looks at" the camera origin from the same Z = 0 point.
    // The only difference is in the horizontal/vertical projection boundaries.
    // TODO: This *might* be worth changing to allow different Z = 0 points per cascade.
    const mat4 shadow_look_at      = glm::lookAt(shadow_cam_position, cam_position, shadow_cam_upvector);
    const mat4 inv_shadow_look_at  = inverse(shadow_look_at);

    // Shadow look_at is a view matrix of shadowcam-space, which is a shadow->world CoB.
    // We use it to transform the contravariant frustum points from world to shadow-space.
    const auto cam_frust_in_shadow_view = frustum_world.transformed(shadow_look_at);

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
        return glm::pow(max_scale, float(split_id + 1) / float(num_cascades));
    };

    auto split_fun_uniform = [&](size_t split_id) {
        return max_scale * float(split_id + 1) / float(num_cascades);
    };

    auto split_fun_practical = [&](size_t split_id, float log_weight, float bias) {
        log_weight = glm::clamp(log_weight, 0.f, 1.f);
        return    (log_weight) * split_fun_log(split_id) +
            (1.f - log_weight) * split_fun_uniform(split_id) + bias;
    };


    out_views.clear();
    const size_t last_cascade_idx = num_cascades - 1;
    for (size_t i{ 0 }; i < num_cascades; ++i) {

        const float split_side = split_fun_practical(i, split_log_weight, split_bias);

        float l = -split_side / 2.f;
        float r = +split_side / 2.f;
        float b = -split_side / 2.f;
        float t = +split_side / 2.f;

        // Size of a single shadowmap texel in shadowmap view-space.
        const vec2 tx_scale{
            split_side / float(resolution.width),
            split_side / float(resolution.height)
        };

        auto snap_to_grid = [&](float& l, float& r, float& b, float& t) {
            // FIXME: This, as any other world-space computation will completely
            // break down when far away from the origin.
            // The addition and subtraction of `center.x` will obliterate
            // the contribution of a small pixel-scale correction fairly quickly too.

            // This is a position of the shadowcam in space that's oriented like
            // the shadow view but centered on the world origin.
            const vec3 center = mat3(shadow_look_at) * shadow_cam_position;

            // Frustum bounds in world-space oriented as shadowcam.
            const float bv = b + center.y;
            const float tv = t + center.y;
            const float lv = l + center.x;
            const float rv = r + center.x;

            // glm::floorMultiple uses subtraction in lower precision space,
            // instead of divide-floor-multiply which floors in higher precision.
            // Kinda suboptimal, oh well, this is broken anyway.
            l = glm::floorMultiple(lv, tx_scale.x) - center.x;
            r = glm::floorMultiple(rv, tx_scale.x) - center.x;
            b = glm::floorMultiple(bv, tx_scale.y) - center.y;
            t = glm::floorMultiple(tv, tx_scale.y) - center.y;

        };

        snap_to_grid(l, r, b, t);

        const mat4 shadow_proj = glm::ortho(l, r, b, t, z_near, z_far);

        const ViewFrustumAsPlanes frustum_world =
            ViewFrustumAsPlanes::make_local_orthographic(l, r, b, t, z_near, z_far)
                .transformed(inv_shadow_look_at);

        const ViewFrustumAsPlanes frustum_padded_world = [&]{
            if (pad_inner_cascades && i != last_cascade_idx) {
                // Padding is only along X and Y.
                const float lp = l + padding_tx * tx_scale.x;
                const float rp = r - padding_tx * tx_scale.x;
                const float bp = b + padding_tx * tx_scale.y;
                const float tp = t - padding_tx * tx_scale.y;
                return ViewFrustumAsPlanes::make_local_orthographic(lp, rp, bp, tp, z_near, z_far)
                    .transformed(inv_shadow_look_at);
            } else {
                return frustum_world;
            }
        }();

        out_views.push_back({
            .params        = {
                .width  = r - l,
                .height = t - b,
                .z_near = z_near,
                .z_far  = z_far
            },
            .tx_scale      = tx_scale,
            .frustum_world = frustum_world,
            .frustum_padded_world = frustum_padded_world,
            .view_mat      = shadow_look_at,
            .proj_mat      = shadow_proj,
            .draw_lists    = {}   // Will be updated later, maybe. (Keep in a separate structure?)
        });

    }
}


void cull_per_cascade(
    std::span<CascadeView> cascades,
    const entt::registry&  registry)
{
    using glm::vec3;
    assert(cascades.size() != 0);

    // Reset all lists first.
    for (auto& cascade : cascades) {
        cascade.draw_lists.visible_at  .clear();
        cascade.draw_lists.visible_noat.clear();
    }

    /*
    Some things that improve the culling:

        1. Test if the object is fully inside of one of the inner cascades.
           If so, discard from drawing it as part of the outer cascade.

        2. Compute texel size for each cascade and discard objects with AABB
           extents smaller than that. (Needs texel size stored).
           NOTE: "Second largest extent" should be a good heuristic.

        3. Using texel size, create "padded" frusti, to support cascade blending.
           Adjust culling according to the padded frustum. This is conservative
           and will result in more draw calls, since both inner and outer cascade
           need to draw an object if it is in the "blend region".

    */

    for (const auto entity : registry.view<MTransform, Mesh, AABB>()) {
        const entt::const_handle handle{ registry, entity };
        const auto& aabb = handle.get<AABB>();

        // We can discard draws for meshes, whose extents are too small
        // to even affect a single shadowmap texel.
        //
        // We best pick the median extent (second largest of three),
        // because other (largest and smallest) have problematic edge cases:
        //
        //  - Using largest extent means that long objects (poles)
        //    don't get culled, even though they can not be represented
        //    in the texels in other dimensions.
        //
        //  - Using smallest extent means that flat objects (planes)
        //    can get culled, even though they are much larger than
        //    the texels in other dimensions.
        //
        const vec3 extents = aabb.extents();

        using std::min, std::max;

        const float median_extent =
            max(min(extents.x, extents.y), min(max(extents.x, extents.y), extents.z));

        using CSMDrawLists = CascadeView::CSMDrawLists;
        using mptr_type = decltype(&CSMDrawLists::visible_at);

        auto test_cascades_and_output_into = [&](mptr_type out_list_mptr) {
            // We abuse the fact that the cascades are stored in order
            // from smallest to largest, where the outer cascades
            // always fully contain the inner ones.
            //
            // If the inner cascades "volume" completely obscures an object from
            // the outer cascade, then we don't render that object to the
            // outer cascade, since it will be sampled from the inner anyway.
            auto cascade_it = cascades.begin();
            do {
                if (cascade_it->tx_scale.x > median_extent) {
                    break; // Too small, discard.
                }

                const auto& padded_frustum = cascade_it->frustum_padded_world;
                const auto& full_frustum   = cascade_it->frustum_world;

                if (is_fully_inside_of(aabb, padded_frustum)) {
                    ((cascade_it->draw_lists).*out_list_mptr).emplace_back(entity);
                    break;
                }

                if (!is_fully_outside_of(aabb, full_frustum)) {
                    ((cascade_it->draw_lists).*out_list_mptr).emplace_back(entity);
                }

                ++cascade_it;
            } while (cascade_it != cascades.end());
        };


        if (has_tag<AlphaTested>(handle) &&
            has_component<MaterialDiffuse>(handle))
        {
            test_cascades_and_output_into(&CSMDrawLists::visible_at);
        } else {
            test_cascades_and_output_into(&CSMDrawLists::visible_noat);
        }
    }

}


} // namespace




void CascadedShadowMapping::operator()(
    RenderEnginePrimaryInterface& engine)
{
    auto& registry = engine.registry();

    // Check if the scene's directional light has shadow-casting enabled.
    if (const auto dlight = get_active<DirectionalLight, Transform, ShadowCasting>(registry)) {

        // Resize maps.
        cascades_->resolution = resolution;
        cascades_->maps.resize(resolution, GLsizei(num_cascades())); // Will only resize on mismatch.

        // Update cascade views.
        if (const auto cam = get_active<Camera, MTransform>(registry)) {

            using glm::vec3;
            // TODO: As everywhere else, use MTf and decompose orientation.
            const vec3 light_dir     = dlight.get<Transform>().orientation() * -Z;
            const auto cam_mtf       = cam.get<MTransform>();
            const vec3 cam_position  = cam_mtf.decompose_position();
            const auto frustum_world = cam.get<Camera>().view_frustum_as_quads().transformed(cam_mtf.model());

            fit_cascade_views_to_camera(
                cascades_->views,
                num_cascades(),
                resolution,
                cam_position,
                frustum_world,
                light_dir,
                split_log_weight,
                split_bias,
                support_cascade_blending,
                blend_size_inner_tx
            );
        }

        // Do the shadowmapping pass.

        if (strategy == Strategy::SinglepassGS) {
            cascades_->draw_lists_active = false;
            draw_all_cascades_with_geometry_shader(engine);
        } else if (strategy == Strategy::PerCascadeCulling) {
            cascades_->draw_lists_active = true;
            cull_per_cascade(cascades_->views, registry);
            draw_with_culling_per_cascade(engine);
        }

        // Pass-through other params.
        cascades_->blend_possible          = support_cascade_blending;
        cascades_->blend_max_size_inner_tx = blend_size_inner_tx;

    }

    // TODO: Whose responsibility it is to set the viewport? Not of this stage tbh.
    glapi::set_viewport({ {}, engine.main_resolution() });
}




namespace {


// Requires that each entity in `entities` has MTransform and Mesh.
//
// Assumes that projection and view uniforms are already set.
void draw_meshes_no_alpha(
    RawProgram<>                        sp,
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const entt::registry&               registry,
    std::ranges::input_range auto&&     entities)
{
    BindGuard bound_program = sp.use();
    const Location model_loc = sp.get_uniform_location("model");

    for (const entt::entity entity : entities) {
        auto [mtf, mesh] = registry.get<MTransform, Mesh>(entity);
        sp.uniform(model_loc, mtf.model());
        mesh.draw(bound_program, bound_fbo);
    }
}


// Requires that each entity in `entities` has MTransform, Mesh and MaterialDiffuse.
// Also, it most likely has to be tagged AlphaTested.
//
// Assumes that projection and view uniforms are already set.
void draw_meshes_with_alpha(
    RawProgram<>                        sp,
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const entt::registry&               registry,
    std::ranges::input_range auto&&     entities)
{
    BindGuard bound_program = sp.use();
    const Location model_loc = sp.get_uniform_location("model");

    sp.uniform("material.diffuse", 0);

    for (const entt::entity entity : entities) {
        auto [mtf, mesh, diffuse] = registry.get<MTransform, Mesh, MaterialDiffuse>(entity);
        diffuse.texture->bind_to_texture_unit(0);
        sp.uniform(model_loc, mtf.model());
        mesh.draw(bound_program, bound_fbo);
    }
}


} // namespace




void CascadedShadowMapping::draw_all_cascades_with_geometry_shader(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();
    auto& maps = cascades_->maps;

    // No following calls are valid for empty cascades array.
    // The framebuffer would be incomplete.
    assert(num_cascades() != 0 && maps.num_array_elements() != 0);


    glapi::set_viewport({ {}, maps.resolution() });
    glapi::enable(Capability::DepthTesting);

    BindGuard bound_fbo = maps.bind_draw();

    glapi::clear_depth_buffer(bound_fbo, 1.f);


    auto set_common_uniforms = [&](RawProgram<> sp) {
        const auto num_cascades = GLsizei(this->num_cascades()); // These conversions are incredible.

        Location proj_loc = sp.get_uniform_location("projections");
        Location view_loc = sp.get_uniform_location("views");

        for (GLsizei cascade_id{ 0 }; cascade_id < num_cascades; ++cascade_id) {
            sp.uniform(Location{ proj_loc + cascade_id }, cascades_->views[cascade_id].proj_mat);
            sp.uniform(Location{ view_loc + cascade_id }, cascades_->views[cascade_id].view_mat);
        }
        sp.uniform("num_cascades", num_cascades);
    };


    if (enable_face_culling) {
        glapi::enable(Capability::FaceCulling);
        glapi::set_face_culling_target(enum_cast<Faces>(faces_to_cull));
    } else {
        glapi::disable(Capability::FaceCulling);
    }

    set_common_uniforms(sp_singlepass_gs_no_alpha_);
    // You could have no AT requested, or you could have an AT flag,
    // but no diffuse material to sample from. Both ignore Alpha-Testing.
    auto no_alpha           = registry.view<MTransform, Mesh>(entt::exclude<AlphaTested>);
    auto with_at_no_diffuse = registry.view<MTransform, Mesh, AlphaTested>(entt::exclude<MaterialDiffuse>);
    draw_meshes_no_alpha(sp_singlepass_gs_no_alpha_, bound_fbo, registry, no_alpha);
    draw_meshes_no_alpha(sp_singlepass_gs_no_alpha_, bound_fbo, registry, with_at_no_diffuse);

    glapi::set_face_culling_target(Faces::Back);
    glapi::disable(Capability::FaceCulling);

    set_common_uniforms(sp_singlepass_gs_with_alpha_);
    auto with_alpha = registry.view<MTransform, Mesh, AlphaTested>();
    draw_meshes_no_alpha(sp_singlepass_gs_with_alpha_, bound_fbo, registry, with_alpha);

}




void CascadedShadowMapping::draw_with_culling_per_cascade(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();
    auto& maps = cascades_->maps;

    // No following calls are valid for empty cascades array.
    // The framebuffer would be incomplete.
    assert(num_cascades() != 0 && maps.num_array_elements() != 0);

    glapi::set_viewport({ {}, maps.resolution() });
    glapi::enable(Capability::DepthTesting);

    for (GLuint cascade_id{ 0 }; cascade_id < num_cascades(); ++cascade_id) {
        const CascadeView& cascade_info = cascades_->views[cascade_id];

        auto set_common_uniforms = [&](RawProgram<> sp) {
            sp.uniform("projection", cascade_info.proj_mat);
            sp.uniform("view",       cascade_info.view_mat);
        };

        cascade_layer_target_.reset_depth_attachment(
            cascades_->maps.share_depth_attachment_layer(Layer(GLsizei(cascade_id)))
        );

        BindGuard bound_fbo = cascade_layer_target_.bind_draw();

        glapi::clear_depth_buffer(bound_fbo, 1.f);

        const auto& draw_lists = cascade_info.draw_lists;

        if (enable_face_culling) {
            glapi::enable(Capability::FaceCulling);
            glapi::set_face_culling_target(enum_cast<Faces>(faces_to_cull));
        } else {
            glapi::disable(Capability::FaceCulling);
        }

        set_common_uniforms(sp_per_cascade_no_alpha_);
        draw_meshes_no_alpha(sp_per_cascade_no_alpha_, bound_fbo, registry, draw_lists.visible_noat);

        glapi::set_face_culling_target(Faces::Back);
        glapi::disable(Capability::FaceCulling);

        set_common_uniforms(sp_per_cascade_with_alpha_);
        draw_meshes_with_alpha(sp_per_cascade_with_alpha_, bound_fbo, registry, draw_lists.visible_at);
    }

}


} // namespace josh::stages::primary
