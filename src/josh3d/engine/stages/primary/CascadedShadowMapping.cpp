#include "CascadedShadowMapping.hpp"
#include "Active.hpp"
#include "Camera.hpp"
#include "Common.hpp"
#include "Components.hpp"
#include "DefaultTextures.hpp"
#include "DrawHelpers.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICore.hpp"
#include "GLProgram.hpp"
#include "GLUniformTraits.hpp"
#include "GeometryCollision.hpp"
#include "LightCasters.hpp"
#include "MeshRegistry.hpp"
#include "MeshStorage.hpp"
#include "Ranges.hpp"
#include "StageContext.hpp"
#include "Scalars.hpp"
#include "components/StaticMesh.hpp"
#include "UploadBuffer.hpp"
#include "VertexFormats.hpp"
#include "ViewFrustum.hpp"
#include "components/AlphaTested.hpp"
#include "components/Materials.hpp"
#include "Transform.hpp"
#include "UniformTraits.hpp"
#include "components/ShadowCasting.hpp"
#include "Tracy.hpp"
#include <algorithm>
#include <ranges>
#include <span>


namespace josh {


CascadedShadowMapping::CascadedShadowMapping(
    i32 side_resolution,
    i32 num_desired_cascades)
{
    assert(side_resolution > 0);
    const i32 num_cascades = _allowed_num_cascades(num_desired_cascades);
    cascades.maps._resize({ side_resolution, side_resolution }, num_cascades);
}

auto CascadedShadowMapping::num_cascades() const noexcept
    -> i32
{
    return cascades.maps.num_cascades();
}

auto CascadedShadowMapping::side_resolution() const noexcept
    -> i32
{
    return cascades.maps.resolution().width;
}

void CascadedShadowMapping::resize_maps(
    i32 side_resolution,
    i32 num_desired_cascades)
{
    assert(side_resolution > 0);
    cascades.maps._resize({ side_resolution, side_resolution }, _allowed_num_cascades(num_desired_cascades));
}

auto CascadedShadowMapping::_allowed_num_cascades(i32 desired_num) const noexcept
    -> i32
{
    return std::clamp(desired_num, 1, max_cascades());
}

auto CascadedShadowMapping::max_cascades() const noexcept
    -> i32
{
    // TODO: Query API limits.
    return 7; // Chosen by a fair rice doll.
}


namespace {

void fit_cascade_views_to_camera(
    Vector<CascadeView>& out_views,
    usize                num_cascades,
    Extent2I             resolution,
    const vec3&          cam_position,
    const FrustumQuads&  frustum_world,
    const vec3&          light_dir,
    float                split_log_weight,
    float                split_bias,
    bool                 pad_inner_cascades,
    float                padding_tx)
{
    ZoneScoped;
    // WARN: This is still heavily WIP.

    // TODO: There's still clipping for the largest cascade.
    const float largest_observable_length = // OR IS IT?
        glm::length(frustum_world.far().points[0] - cam_position);

    const float z_near = 0.f;
    const float z_far  = 2 * largest_observable_length;

    // Similar to cam_offset in simple shadow mapping.
    const float cam_offset = (z_far - z_near) / 2.f;

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
    const mat4 shadow_look_at     = glm::lookAt(shadow_cam_position, cam_position, shadow_cam_upvector);
    const mat4 inv_shadow_look_at = inverse(shadow_look_at);

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
    constexpr float inf = std::numeric_limits<float>::infinity();
    vec2 min{  inf,  inf };
    vec2 max{ -inf, -inf };
    {
        auto minmax_update_from_corner = [&](const vec3& corner)
        {
            min.x = std::min(min.x, corner.x);
            min.y = std::min(min.y, corner.y);
            max.x = std::max(max.x, corner.x);
            max.y = std::max(max.y, corner.y);
        };

        for (const vec3& corner : cam_frust_in_shadow_view.near().points)
            minmax_update_from_corner(corner);

        for (const vec3& corner : cam_frust_in_shadow_view.far().points)
            minmax_update_from_corner(corner);
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
    auto split_fun_log = [&](uindex split_id)
    {
        return glm::pow(max_scale, float(split_id + 1) / float(num_cascades));
    };

    auto split_fun_uniform = [&](uindex split_id)
    {
        return max_scale * float(split_id + 1) / float(num_cascades);
    };

    auto split_fun_practical = [&](uindex split_id, float log_weight, float bias)
    {
        log_weight = glm::clamp(log_weight, 0.f, 1.f);
        return
            (log_weight)       * split_fun_log(split_id) +
            (1.f - log_weight) * split_fun_uniform(split_id) + bias;
    };

    out_views.clear();
    const uindex last_cascade_idx = num_cascades - 1;
    for (const uindex i : irange(num_cascades))
    {
        const float split_side = split_fun_practical(i, split_log_weight, split_bias);

        float l = -split_side / 2.f;
        float r = +split_side / 2.f;
        float b = -split_side / 2.f;
        float t = +split_side / 2.f;

        // Size of a single shadowmap texel in shadowmap view-space.
        const vec2 tx_scale = {
            split_side / float(resolution.width),
            split_side / float(resolution.height)
        };

        auto snap_to_grid = [&](float& l, float& r, float& b, float& t)
        {
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

        const FrustumPlanes frustum_world =
            FrustumPlanes::make_local_orthographic(l, r, b, t, z_near, z_far)
                .transformed(inv_shadow_look_at);

        const FrustumPlanes frustum_padded_world = eval%[&]{
            if (pad_inner_cascades and i != last_cascade_idx)
            {
                // Padding is only along X and Y.
                const float lp = l + padding_tx * tx_scale.x;
                const float rp = r - padding_tx * tx_scale.x;
                const float bp = b + padding_tx * tx_scale.y;
                const float tp = t - padding_tx * tx_scale.y;
                return FrustumPlanes::make_local_orthographic(lp, rp, bp, tp, z_near, z_far)
                    .transformed(inv_shadow_look_at);
            }
            else
            {
                return frustum_world;
            }
        };

        out_views.push_back({
            .width         = r - l,
            .height        = t - b,
            .z_near        = z_near,
            .z_far         = z_far,
            .tx_scale      = tx_scale,
            .frustum_world = frustum_world,
            .frustum_padded_world = frustum_padded_world,
            .view_mat      = shadow_look_at,
            .proj_mat      = shadow_proj,
        });
    }
}

void cull_per_cascade(
    Span<const CascadeView> views,
    Span<CascadeDrawState>  drawstates,
    const Registry&         registry)
{
    ZS;
    assert(views.size() != 0);
    assert(views.size() == drawstates.size());
    const usize num_cascades = views.size();

    // Reset all lists first.
    for (auto& drawstate : drawstates)
    {
        drawstate.drawlist_atested     .clear();
        drawstate.drawlist_opaque .clear();
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

    for (const Entity entity : registry.view<MTransform, StaticMesh, AABB>())
    {
        const CHandle handle = { registry, entity };
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
        using std::min, std::max;

        const vec3 extents = aabb.extents();

        const float median_extent =
            max(min(extents.x, extents.y), min(max(extents.x, extents.y), extents.z));

        // TODO: Is there really not a better way?
        using memptr_type = decltype(&CascadeDrawState::drawlist_atested);

        const auto test_cascades_and_output_into = [&](memptr_type out_list_mptr)
        {
            // We abuse the fact that the cascades are stored in order
            // from smallest to largest, where the outer cascades
            // always fully contain the inner ones.
            //
            // If the inner cascades "volume" completely obscures an object from
            // the outer cascade, then we don't render that object to the
            // outer cascade, since it will be sampled from the inner anyway.

            for (const uindex i : irange(num_cascades))
            {
                const auto& view      = views     [i];
                auto&       drawstate = drawstates[i];
                auto&       drawlist  = drawstate.*out_list_mptr;

                if (view.tx_scale.x > median_extent)
                    break; // Too small, discard.

                const auto& padded_frustum = view.frustum_padded_world;
                const auto& full_frustum   = view.frustum_world;

                if (is_fully_inside_of(aabb, padded_frustum))
                {
                    drawlist.emplace_back(entity);
                    break;
                }

                if (not is_fully_outside_of(aabb, full_frustum))
                    drawlist.emplace_back(entity);
            }
        };

        if (has_tag<AlphaTested>(handle) and has_component<MaterialPhong>(handle))
            test_cascades_and_output_into(&CascadeDrawState::drawlist_atested);
        else
            test_cascades_and_output_into(&CascadeDrawState::drawlist_opaque);
    }
}

} // namespace


void CascadedShadowMapping::operator()(
    PrimaryContext context)
{
    ZSCGPUN("CSM");
    const auto& registry = context.registry();

    const CHandle dlight = get_active<DirectionalLight, ShadowCasting, MTransform>(registry);
    const CHandle cam    = get_active<Camera, MTransform>(registry);

    if (not dlight) return;
    if (not cam)    return;

    const vec3 light_dir     = decompose_rotation(dlight.get<MTransform>()) * -Z;
    const auto cam_mtf       = cam.get<MTransform>();
    const vec3 cam_position  = cam_mtf.decompose_position();
    const auto frustum_world = cam.get<Camera>().view_frustum_as_quads().transformed(cam_mtf.model());

    fit_cascade_views_to_camera(
        cascades.views,
        num_cascades(),
        cascades.maps.resolution(),
        cam_position,
        frustum_world,
        light_dir,
        split_log_weight,
        split_bias,
        support_cascade_blending,
        blend_size_inner_tx
    );

    // Resize drawstates if necessary.
    cascades.drawstates.resize(num_cascades());

    // Do the shadowmapping pass.
    if (strategy == Strategy::SinglepassGS)
    {
        cascades.draw_lists_active = false;
        _draw_all_cascades_with_geometry_shader(context);
    }

    if (strategy == Strategy::PerCascadeCulling or
        strategy == Strategy::PerCascadeCullingMDI)
    {
        cascades.draw_lists_active = true;
        cull_per_cascade(cascades.views, cascades.drawstates, registry);
        // NOTE: Will select single or MDI based on the enum value.
        _draw_with_culling_per_cascade(context);
    }

    // Pass-through other params.
    cascades.blend_possible          = support_cascade_blending;
    cascades.blend_max_size_inner_tx = blend_size_inner_tx;

    context.belt().put_ref(cascades);
}


namespace {

/*
Requires that each entity in `entities` has MTransform and StaticMesh.

Assumes that projection and view uniforms are already set.
*/
void draw_opaque_meshes(
    RawProgram<>                        sp,
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const MeshRegistry&                 mesh_registry,
    const Registry&                     registry,
    std::ranges::input_range auto&&     entities)
{
    const auto* storage = mesh_registry.storage_for<VertexStatic>();
    assert(storage);

    const BindGuard bsp = sp.use();
    const BindGuard bva = storage->vertex_array().bind();

    const Location model_loc = sp.get_uniform_location("model");

    for (const Entity entity : entities)
    {
        auto [mtf, mesh] = registry.get<MTransform, StaticMesh>(entity);
        sp.uniform(model_loc, mtf.model());
        draw_one_from_storage(*storage, bva, bsp, bound_fbo, mesh.lods.cur());
    }
}

/*
Requires that each entity in `entities` has MTransform and StaticMesh.
Also, it most likely has to have MaterialPhong and be tagged AlphaTested.

Assumes that projection and view uniforms are already set.
*/
void draw_atested_meshes(
    RawProgram<>                        sp,
    BindToken<Binding::DrawFramebuffer> bound_fbo,
    const MeshRegistry&                 mesh_registry,
    const Registry&                     registry,
    std::ranges::input_range auto&&     entities)
{
    const auto* storage = mesh_registry.storage_for<VertexStatic>();
    assert(storage);

    const BindGuard bsp = sp.use();
    const BindGuard bva = storage->vertex_array().bind();

    sp.uniform("material.diffuse", 0);

    const Location model_loc = sp.get_uniform_location("model");

    for (const Entity entity : entities)
    {
        auto [mtf, mesh] = registry.get<MTransform, StaticMesh>(entity);

        if (auto* mtl = registry.try_get<MaterialPhong>(entity))
            mtl->diffuse->bind_to_texture_unit(0);
        else
            globals::default_diffuse_texture().bind_to_texture_unit(0);

        sp.uniform(model_loc, mtf.model());
        draw_one_from_storage(*storage, bva, bsp, bound_fbo, mesh.lods.cur());
    }
}


} // namespace


void CascadedShadowMapping::_draw_all_cascades_with_geometry_shader(
    PrimaryContext context)
{
    const auto& registry      = context.registry();
    const auto& mesh_registry = context.mesh_registry();
    auto& maps = cascades.maps;

    // No following calls are valid for empty cascades array.
    // The framebuffer would be incomplete. This should be accounted for before.
    assert(num_cascades() > 0);

    glapi::set_viewport({ {}, maps.resolution() });
    glapi::enable(Capability::DepthTesting);

    _fbo->attach_texture_to_depth_buffer(maps.textures());
    const BindGuard bfb = _fbo->bind_draw();

    glapi::clear_depth_buffer(bfb, 1.f);

    const auto set_common_uniforms = [&](RawProgram<> sp)
    {
        const auto num_cascades = GLsizei(this->num_cascades()); // These conversions are incredible.

        const Location proj_loc = sp.get_uniform_location("projections");
        const Location view_loc = sp.get_uniform_location("views");

        for (GLsizei cascade_id = 0; cascade_id < num_cascades; ++cascade_id)
        {
            sp.uniform(Location{ proj_loc + cascade_id }, cascades.views[cascade_id].proj_mat);
            sp.uniform(Location{ view_loc + cascade_id }, cascades.views[cascade_id].view_mat);
        }
        sp.uniform("num_cascades", num_cascades);
    };

    // SinglepassGS - Opaque.
    {
        const RawProgram<> sp = _sp_opaque_singlepass_gs.get();

        if (enable_face_culling)
        {
            glapi::enable(Capability::FaceCulling);
            glapi::set_face_culling_target(enum_cast<Faces>(faces_to_cull));
        }
        else
        {
            glapi::disable(Capability::FaceCulling);
        }

        set_common_uniforms(sp);

        // You could have no AT requested, or you could have an AT flag,
        // but no diffuse material to sample from. Both ignore Alpha-Testing.

        // TODO: This are negative filters. Negative filters are *not* fast.
        auto no_alpha = registry.view<MTransform, StaticMesh>(entt::exclude<AlphaTested>);
        draw_opaque_meshes(sp, bfb, mesh_registry, registry, no_alpha);
    }

    // SinglepassGS - Alpha
    {
        const RawProgram<> sp = _sp_atested_singlepass_gs.get();

        glapi::set_face_culling_target(Faces::Back);
        glapi::disable(Capability::FaceCulling);

        set_common_uniforms(sp);

        auto with_alpha = registry.view<AlphaTested, MTransform, StaticMesh>();
        draw_atested_meshes(sp, bfb, mesh_registry, registry, with_alpha);
    }
}



namespace {

/*
Requires that each entity in `entities` has MTransform and StaticMesh.

Assumes that projection and view uniforms are already set.
*/
void multidraw_opaque_meshes(
    RawProgram<>                        sp,
    BindToken<Binding::DrawFramebuffer> bfb,
    const MeshRegistry&                 mesh_registry,
    const Registry&                     registry,
    std::ranges::input_range auto&&     entities,
    UploadBuffer<mat4>&                 world_mats,
    UploadBuffer<MDICommand>&           mdi_buffer)
{
    const auto* storage = mesh_registry.storage_for<VertexStatic>();
    assert(storage);

    const BindGuard bsp = sp.use();
    const BindGuard bva = storage->vertex_array().bind();

    auto get_mesh_id   = [&](Entity e) -> decltype(auto) { return registry.get<StaticMesh>(e).lods.cur(); };
    auto get_world_mat = [&](Entity e) -> decltype(auto) { return registry.get<MTransform>(e).model();  };

    // Prepare world matrices for all drawable objects.
    world_mats.restage(entities | transform(get_world_mat));
    world_mats.bind_to_ssbo_index(0);

    // Draw all at once.
    //
    // NOTE: Mesa gl_DrawID is still broken for direct multidraw.
    // Use MDI to avoid issues.
    multidraw_indirect_from_storage(*storage, bva, bsp, bfb,
        entities | transform(get_mesh_id), mdi_buffer);
}
/*
Requires that each entity in `entities` has MaterialPhong, MTransform and StaticMesh.

Assumes that projection and view uniforms are already set.
*/
void multidraw_atested_meshes(
    RawProgram<>                        sp,
    BindToken<Binding::DrawFramebuffer> bfb,
    const MeshRegistry&                 mesh_registry,
    const Registry&                     registry,
    std::ranges::input_range auto&&     entities,
    UploadBuffer<mat4>&                 instance_data,
    UploadBuffer<MDICommand>&           mdi_buffer)
{
    const auto* storage = mesh_registry.storage_for<VertexStatic>();
    assert(storage);

    const BindGuard bsp = sp.use();
    const BindGuard bva = storage->vertex_array().bind();

    const usize num_units  = max_frag_texture_units();
    const usize batch_size = max_frag_texture_units();

    const Span<const i32> samplers = build_irange_tls_array(num_units);
    sp.set_uniform_intv(sp.get_uniform_location("samplers"), i32(samplers.size()), samplers.data());

    thread_local Vector<u32>          tex_units; tex_units.clear();
    thread_local Vector<StaticMeshID> mesh_ids;  mesh_ids.clear();

    // NOTE: Not clearing instance data between draws since we want
    // to preserve "history", will track and bind subranges instead.
    instance_data.clear();
    usize  batch_offset = 0;
    uindex draw_id      = 0;

    const auto push_instance = [&](Entity e)
    {
        instance_data.stage_one(registry.get<MTransform>(e).model());
        tex_units    .push_back(registry.get<MaterialPhong>(e).diffuse->id());
        mesh_ids     .push_back(registry.get<StaticMesh>(e).lods.cur());
    };

    const auto draw_staged_and_reset = [&]()
    {
        glapi::bind_texture_units(tex_units);
        instance_data.bind_range_to_ssbo_index({ .offset=batch_offset, .count=draw_id }, 0);

        multidraw_indirect_from_storage(*storage, bva, bsp, bfb, mesh_ids, mdi_buffer);

        mesh_ids.clear();
        batch_offset += draw_id;
        draw_id       = 0;
    };

    for (const Entity e : entities)
    {
        push_instance(e);
        if (++draw_id >= batch_size)
            draw_staged_and_reset();
    }
    if (draw_id) draw_staged_and_reset();
}

} // namespace


void CascadedShadowMapping::_draw_with_culling_per_cascade(
    PrimaryContext context)
{
    const auto& registry      = context.registry();
    const auto& mesh_registry = context.mesh_registry();
    auto& maps = cascades.maps;

    // No following calls are valid for empty cascades array.
    // The framebuffer would be incomplete.
    assert(num_cascades() > 0);

    // FIXME: Just clear the texture? No?
    //
    // This is much faster than clearing each layer one-by-one. Don't do that.
    _fbo->attach_texture_to_depth_buffer(cascades.maps.textures());
    _fbo->clear_depth(1.f);

    glapi::set_viewport({ {}, maps.resolution() });
    glapi::enable(Capability::DepthTesting);

    for (const uindex cascade_idx : irange(num_cascades()))
    {
        const Layer cascade_layer = i32(cascade_idx);
        const auto& view          = cascades.views[cascade_idx];
        auto&       drawstate     = cascades.drawstates[cascade_idx];

        const auto set_common_uniforms = [&](RawProgram<> sp)
        {
            sp.uniform("projection", view.proj_mat);
            sp.uniform("view",       view.view_mat);
        };

        // Attach layer-by-layer.
        _fbo->attach_texture_layer_to_depth_buffer(maps.textures(), cascade_layer);
        const BindGuard bfb = _fbo->bind_draw();

        // Draw opaque.
        if (enable_face_culling)
        {
            glapi::enable(Capability::FaceCulling);
            glapi::set_face_culling_target(enum_cast<Faces>(faces_to_cull));
        }
        else
        {
            glapi::disable(Capability::FaceCulling);
        }

        if (strategy == Strategy::PerCascadeCulling)
        {
            const RawProgram<> sp = _sp_opaque_per_cascade;
            set_common_uniforms(sp);
            draw_opaque_meshes(sp, bfb, mesh_registry, registry, drawstate.drawlist_opaque);
        }
        else if (strategy == Strategy::PerCascadeCullingMDI)
        {
            const RawProgram<> sp = _sp_opaque_mdi;
            set_common_uniforms(sp);
            multidraw_opaque_meshes(sp, bfb, mesh_registry, registry,
                drawstate.drawlist_opaque, drawstate.world_mats_opaque, _mdi_buffer);
        }

        // Draw AlphaTested.
        glapi::set_face_culling_target(Faces::Back);
        glapi::disable(Capability::FaceCulling);
        if (strategy == Strategy::PerCascadeCulling)
        {
            const RawProgram<> sp = _sp_atested_per_cascade;
            set_common_uniforms(sp);
            draw_atested_meshes(sp, bfb, mesh_registry, registry, drawstate.drawlist_atested);
        }
        else if (strategy == Strategy::PerCascadeCullingMDI)
        {
            const RawProgram<> sp = _sp_atested_mdi;
            set_common_uniforms(sp);
            multidraw_atested_meshes(sp, bfb, mesh_registry, registry,
                drawstate.drawlist_atested, drawstate.world_mats_atested, _mdi_buffer);
        }
    }
}


} // namespace josh::stages::primary
