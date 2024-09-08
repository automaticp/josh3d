#include "DeferredShading.hpp"
#include "Active.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLProgram.hpp"
#include "LightsGPU.hpp"
#include "Mesh.hpp"
#include "Tags.hpp"
#include "Transform.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
#include "BoundingSphere.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "tags/ShadowCasting.hpp"
#include "RenderEngine.hpp"
#include "tags/Visible.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>
#include <glbinding/gl/gl.h>
#include <range/v3/all.hpp>
#include <range/v3/view/map.hpp>
#include <ranges>




namespace josh::stages::primary {
namespace {


struct ALight {
    glm::vec3 color;
};


auto get_active_alight_or_default(const entt::registry& registry)
    -> ALight
{
    glm::vec3 color{ 0.f, 0.f, 0.f };
    if (const auto alight = get_active<AmbientLight>(registry)) {
        color = alight.get<AmbientLight>().color;
    }
    return { color };
}



struct DLight {
    glm::vec3 color;
    glm::vec3 direction;
    bool      cast_shadows;
};


auto get_active_dlight_or_default(const entt::registry& registry)
    -> DLight
{
    glm::vec3 color       { 0.f, 0.f, 0.f };
    glm::vec3 direction   { 1.f, 1.f, 1.f };
    bool      cast_shadows{ false };

    if (const auto dlight = get_active<DirectionalLight, Transform>(registry)) {
        color        = dlight.get<DirectionalLight>().color;
        direction    = dlight.get<Transform>().orientation() * glm::vec3{ 0.f, 0.f, -1.f };
        cast_shadows = has_tag<ShadowCasting>(dlight);
    }
    return { color, direction, cast_shadows };
}


} // namespace




void DeferredShading::operator()(
    RenderEnginePrimaryInterface& engine)
{
    update_cascade_buffer();
    update_point_light_buffers(engine.registry());

    if (mode == Mode::SinglePass) {
        draw_singlepass(engine);
    } else if (mode == Mode::MultiPass) {
        draw_multipass (engine);
    }
}




void DeferredShading::draw_singlepass(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();
    BindGuard bound_camera_ubo = engine.bind_camera_ubo();

    const RawProgram<> sp = sp_singlepass_;

    // GBuffer.
    gbuffer_->depth_texture()   .bind_to_texture_unit(0);
    gbuffer_->normals_texture() .bind_to_texture_unit(1);
    gbuffer_->albedo_texture()  .bind_to_texture_unit(2);
    gbuffer_->specular_texture().bind_to_texture_unit(3);
    MultibindGuard bound_gbuffer_samplers{
        target_sampler_->bind_to_texture_unit(0),
        target_sampler_->bind_to_texture_unit(1),
        target_sampler_->bind_to_texture_unit(2),
        target_sampler_->bind_to_texture_unit(3),
    };

    sp.uniform("gbuffer.tex_depth",    0);
    sp.uniform("gbuffer.tex_normals",  1);
    sp.uniform("gbuffer.tex_albedo",   2);
    sp.uniform("gbuffer.tex_specular", 3);

    // AO.
    input_ao_->blurred_texture                   .bind_to_texture_unit(5);
    BindGuard bound_ao_sampler = target_sampler_->bind_to_texture_unit(5);
    sp.uniform("tex_ambient_occlusion",   5);
    sp.uniform("use_ambient_occlusion",   use_ambient_occlusion);
    sp.uniform("ambient_occlusion_power", ambient_occlusion_power);

    // Ambient light.
    {
        const auto [color] = get_active_alight_or_default(registry);
        sp.uniform("ambient_light.color", color);
    }

    // Directional light.
    {
        const auto [color, direction, cast_shadows] = get_active_dlight_or_default(registry);
        sp.uniform("dir_light.color",     color);
        sp.uniform("dir_light.direction", direction);
        sp.uniform("dir_shadow.do_cast",  cast_shadows);
    }

    // Directional shadows.
    input_csm_->maps.depth_attachment().texture().bind_to_texture_unit(4);
    BindGuard bound_csm_sampler =   csm_sampler_->bind_to_texture_unit(4);
    sp.uniform("dir_shadow.cascades",            4);
    sp.uniform("dir_shadow.base_bias_tx",        dir_params.base_bias_tx);
    sp.uniform("dir_shadow.do_blend_cascades",   input_csm_->blend_possible);
    sp.uniform("dir_shadow.blend_size_inner_tx", input_csm_->blend_max_size_inner_tx);
    sp.uniform("dir_shadow.pcf_extent",          dir_params.pcf_extent);
    sp.uniform("dir_shadow.pcf_offset",          dir_params.pcf_offset);
    csm_views_buf_.bind_to_ssbo_index(3);

    // Point lights.
    sp.uniform("fade_start_fraction",  plight_fade_start_fraction);
    sp.uniform("fade_length_fraction", plight_fade_length_fraction);
    plights_with_shadow_buf_.bind_to_ssbo_index(1);
    plights_no_shadow_buf_  .bind_to_ssbo_index(2);

    // Point light shadows.
    input_psm_->maps.depth_attachment().texture().bind_to_texture_unit(6);
    BindGuard bound_psm_sampler =   psm_sampler_->bind_to_texture_unit(6);
    sp.uniform("point_shadow.maps",        6);
    sp.uniform("point_shadow.bias_bounds", point_params.bias_bounds);
    sp.uniform("point_shadow.pcf_extent",  point_params.pcf_extent);
    sp.uniform("point_shadow.pcf_offset",  point_params.pcf_offset);



    glapi::disable(Capability::DepthTesting);
    {
        BindGuard bound_program = sp.use();
        engine.draw([&](auto bound_fbo) {
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        });
    }
    glapi::enable(Capability::DepthTesting);

    // The depth buffer is probably shared between the GBuffer
    // and the main framebuffer.
    //
    // This is okay if the deferred shading algorithm does not depend
    // on the depth value. That is, if you need to isolate the
    // depth that was drawn only in deferred passes, then you might
    // have to do just that. And then do some kind of depth blending.

}



// TODO: This entire approach is not really viable due to the enormous
// bandwidth overhead it creates.
//
// In singlepass we sample the GBuffer once per fragment, and just
// iterate over the list of point lights in the scene to compute irradiance.
//
// In multipass we instance draw light volumes (spheres) for the point lights,
// which gives us decent frustum and occlusion culling per-light, at the cost
// of re-sampling the GBuffer PER-LIGHT SOURCE. If you have N lights illuminating
// a single fragment on the screen, then you are doing N-1 redundant samples
// of the GBuffer.
//
// So in case there are many lights *on the screen*, multipass is bandwidth heavy
// and will likely be much slower that singlepass (especially on my poor iGPU).
//
// In case where most of the lights are *off the screen*, multipass is probably faster
// due to culling, although even singlepass can be accompanied by at least basic
// frustim culling on the CPU.
//
// One way to reduce the bandwidth a little is to store the irradiance of each fragment
// in an intermediate buffer, because computing that only requires normals and roughness
// of the surface, and not albedo. But this is not really the solution.
//
// The much more viable solution is to go full in on clustered shading instead,
// which has the same bandwidth requirements as singlepass - each fragment samples GBuffer once,
// but selectively culls the light volumes per-cluster. Not sure about occlusion culling there though.
//
// Either way, I'm leaving this implementation here for now,
// so that it could be used as a stepping stone / testbed for other stuff.
void DeferredShading::draw_multipass(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry   = engine.registry();
    BindGuard bound_camera = engine.bind_camera_ubo();

    gbuffer_->depth_texture()   .bind_to_texture_unit(0);
    gbuffer_->normals_texture() .bind_to_texture_unit(1);
    gbuffer_->albedo_texture()  .bind_to_texture_unit(2);
    gbuffer_->specular_texture().bind_to_texture_unit(3);
    MultibindGuard bound_gbuffer_samplers{
        target_sampler_->bind_to_texture_unit(0),
        target_sampler_->bind_to_texture_unit(1),
        target_sampler_->bind_to_texture_unit(2),
        target_sampler_->bind_to_texture_unit(3),
    };

    auto set_common_uniforms = [&](RawProgram<> sp) {
        sp.uniform("gbuffer.tex_depth",    0);
        sp.uniform("gbuffer.tex_normals",  1);
        sp.uniform("gbuffer.tex_albedo",   2);
        sp.uniform("gbuffer.tex_specular", 3);
    };

    // Ambient + Directional Light Pass.
    {
        const RawProgram<> sp = sp_pass_ambi_dir_;
        BindGuard bound_program = sp.use();

        set_common_uniforms(sp);

        // Ambient Light.
        {
            const auto [color] = get_active_alight_or_default(registry);
            sp.uniform("ambi_light.color", color);
        }

        // Ambient Occlusion.
        input_ao_->blurred_texture                   .bind_to_texture_unit(4);
        BindGuard bound_ao_sampler = target_sampler_->bind_to_texture_unit(4);
        sp.uniform("ambi_occlusion.tex_occlusion", 4);
        sp.uniform("ambi_occlusion.use",           use_ambient_occlusion);
        sp.uniform("ambi_occlusion.power",         ambient_occlusion_power);

        // Directional Light.
        {
            const auto [color, direction, cast_shadows] = get_active_dlight_or_default(registry);
            sp.uniform("dir_light.color",     color);
            sp.uniform("dir_light.direction", direction);
            sp.uniform("dir_shadow.do_cast",  cast_shadows);
        }

        // CSM.
        input_csm_->maps.depth_attachment().texture().bind_to_texture_unit(5);
        BindGuard bound_csm_sampler =   csm_sampler_->bind_to_texture_unit(5);
        sp.uniform("dir_shadow.cascades",            5);
        sp.uniform("dir_shadow.base_bias_tx",        dir_params.base_bias_tx);
        sp.uniform("dir_shadow.do_blend_cascades",   input_csm_->blend_possible);
        sp.uniform("dir_shadow.blend_size_inner_tx", input_csm_->blend_max_size_inner_tx);
        sp.uniform("dir_shadow.pcf_extent",          dir_params.pcf_extent);
        sp.uniform("dir_shadow.pcf_offset",          dir_params.pcf_offset);
        csm_views_buf_.bind_to_ssbo_index(0);


        engine.draw([&](auto bound_fbo) {
            glapi::disable(Capability::DepthTesting);
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
            glapi::enable(Capability::DepthTesting);
        });
    }


    auto set_plight_uniforms = [&](RawProgram<> sp) {
        sp.uniform("fade_start_fraction",  plight_fade_start_fraction);
        sp.uniform("fade_length_fraction", plight_fade_length_fraction);
    };


    auto instance_draw_plight_spheres = [&](
        GLsizeiptr instance_count,
        auto       bound_program)
    {
        engine.draw([&](auto bound_fbo) {
            glapi::enable(Capability::DepthTesting);
            glapi::set_depth_test_condition(CompareOp::Greater);
            glapi::set_depth_mask(false);

            glapi::enable(Capability::FaceCulling);
            glapi::set_face_culling_target(Faces::Front);

            glapi::enable(Capability::Blending);
            glapi::set_blend_factors(BlendFactor::One, BlendFactor::One);


            const auto& mesh = engine.primitives().sphere_mesh();
            BindGuard bound_vao = mesh.vertex_array().bind();

            glapi::draw_elements_instanced(
                bound_vao,
                bound_program,
                bound_fbo,
                instance_count,
                mesh.primitive_type(),
                mesh.element_type(),
                mesh.element_offset_bytes(),
                mesh.num_elements()
            );


            glapi::disable(Capability::Blending);

            glapi::set_face_culling_target(Faces::Back);
            glapi::disable(Capability::FaceCulling);

            glapi::set_depth_test_condition(CompareOp::Less);
            glapi::set_depth_mask(true);
            glapi::enable(Capability::DepthTesting);
        });
    };


    // Point Lights No Shadows Pass.
    if (plights_no_shadow_buf_.num_staged() != 0) {
        const RawProgram<> sp = sp_pass_plight_no_shadow_;
        BindGuard bound_program = sp.use();

        set_common_uniforms(sp);
        set_plight_uniforms(sp);

        plights_no_shadow_buf_.bind_to_ssbo_index(0);

        instance_draw_plight_spheres(plights_no_shadow_buf_.num_staged(), bound_program.token());
    }

    // Point Lights With Shadows Pass.
    if (plights_with_shadow_buf_.num_staged() != 0) {
        const RawProgram<> sp = sp_pass_plight_with_shadow_;
        BindGuard bound_program = sp.use();

        set_common_uniforms(sp);
        set_plight_uniforms(sp);

        plights_with_shadow_buf_.bind_to_ssbo_index(0);

        // Point Shadows.
        input_psm_->maps.depth_attachment().texture().bind_to_texture_unit(4);
        BindGuard bound_psm_sampler =   psm_sampler_->bind_to_texture_unit(4);
        sp.uniform("point_shadows.maps",        4);
        sp.uniform("point_shadows.bias_bounds", point_params.bias_bounds);
        sp.uniform("point_shadows.pcf_extent",  point_params.pcf_extent);
        sp.uniform("point_shadows.pcf_offset",  point_params.pcf_offset);

        instance_draw_plight_spheres(plights_with_shadow_buf_.num_staged(), bound_program.token());
    }



}




void DeferredShading::update_cascade_buffer() {
    csm_views_buf_.restage(input_csm_->views | std::views::transform(CascadeViewGPU::create_from));
}


void DeferredShading::update_point_light_buffers(
    const entt::registry& registry)
{
    // TODO: Uhh, how do I know that the order of lights in the view is the same as the
    // order of shadow cubemaps in `input_psm_->maps`?
    auto plights_with_shadow_view = registry.view<Visible, MTransform, PointLight, BoundingSphere, ShadowCasting>();
    auto plights_no_shadow_view   = registry.view<Visible, MTransform, PointLight, BoundingSphere>(entt::exclude<ShadowCasting>);

    // From tuple<MTransform, PointLight, BoundingSphere> to combined GPU-layout structure.
    auto repack = [](auto tuple) {
        const auto& [_, mtf, plight, sphere] = tuple;
        return PointLightBoundedGPU{
            .color       = plight.color,
            .position    = mtf.decompose_position(),
            .radius      = sphere.radius,
            .attenuation = {
                .constant  = plight.attenuation.constant,
                .linear    = plight.attenuation.linear,
                .quadratic = plight.attenuation.quadratic
            }
        };
    };

    plights_with_shadow_buf_.restage(plights_with_shadow_view.each() | std::views::transform(repack));
    plights_no_shadow_buf_  .restage(plights_no_shadow_view.each()   | std::views::transform(repack));

}


} // namespace josh::stages::primary
