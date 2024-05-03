#include "DeferredShading.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLProgram.hpp"
#include "Mesh.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
#include "components/BoundingSphere.hpp"
#include "tags/ShadowCasting.hpp"
#include "RenderEngine.hpp"
#include <algorithm>
#include <entt/entity/registry.hpp>
#include <glbinding/gl/gl.h>
#include <range/v3/all.hpp>
#include <range/v3/view/map.hpp>




namespace josh::stages::primary {


void DeferredShading::operator()(
    RenderEnginePrimaryInterface& engine)
{
    update_cascade_buffer();

    auto [num_plights_with_shadow, num_plights_no_shadow] = update_point_light_buffers(engine.registry());

    if (mode == Mode::SinglePass) {
        draw_singlepass(engine, num_plights_with_shadow, num_plights_no_shadow);
    } else if (mode == Mode::MultiPass) {
        draw_multipass (engine, num_plights_with_shadow, num_plights_no_shadow);
    }
}


void DeferredShading::draw_singlepass(
    RenderEnginePrimaryInterface& engine,
    NumElems                      num_plights_with_shadow, // TODO: Is there no better way to communicate this?
    NumElems                      num_plights_no_shadow)
{
    const auto& registry = engine.registry();


    // GBuffer.
    gbuffer_->position_draw_texture().bind_to_texture_unit(0);
    gbuffer_->normals_texture()      .bind_to_texture_unit(1);
    gbuffer_->albedo_spec_texture()  .bind_to_texture_unit(2);
    MultibindGuard bound_gbuffer_samplers{
        target_sampler_             ->bind_to_texture_unit(0),
        target_sampler_             ->bind_to_texture_unit(1),
        target_sampler_             ->bind_to_texture_unit(2),
    };
    sp_singlepass_->uniform("tex_position_draw", 0);
    sp_singlepass_->uniform("tex_normals",       1);
    sp_singlepass_->uniform("tex_albedo_spec",   2);

    // AO.
    input_ao_->blurred_texture                   .bind_to_texture_unit(5);
    BindGuard bound_ao_sampler = target_sampler_->bind_to_texture_unit(5);
    sp_singlepass_->uniform("tex_ambient_occlusion",   5);
    sp_singlepass_->uniform("use_ambient_occlusion",   use_ambient_occlusion);
    sp_singlepass_->uniform("ambient_occlusion_power", ambient_occlusion_power);

    // Ambient light.
    auto& alight = *registry.storage<light::Ambient>().begin();
    sp_singlepass_->uniform("ambient_light.color", alight.color);

    // Directional light.
    auto [dlight_ent, dlight] = *registry.view<light::Directional>().each().begin();
    sp_singlepass_->uniform("dir_light.color",        dlight.color);
    sp_singlepass_->uniform("dir_light.direction",    dlight.direction);
    sp_singlepass_->uniform("dir_shadow.do_cast",     registry.all_of<tags::ShadowCasting>(dlight_ent));

    // Directional shadows.
    input_csm_->dir_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(3);
    BindGuard bound_csm_sampler =                  csm_sampler_->bind_to_texture_unit(3);
    sp_singlepass_->uniform("dir_shadow.cascades",            3);
    sp_singlepass_->uniform("dir_shadow.base_bias_tx",        dir_params.base_bias_tx);
    sp_singlepass_->uniform("dir_shadow.do_blend_cascades",   dir_params.blend_cascades);
    sp_singlepass_->uniform("dir_shadow.blend_size_inner_tx", dir_params.blend_size_inner_tx);
    sp_singlepass_->uniform("dir_shadow.pcf_extent",          dir_params.pcf_extent);
    sp_singlepass_->uniform("dir_shadow.pcf_offset",          dir_params.pcf_offset);
    csm_params_buf_->bind_to_index<BufferTargetIndexed::ShaderStorage>(3);

    // Point lights.
    sp_singlepass_->uniform("fade_start_fraction",  plight_fade_start_fraction);
    sp_singlepass_->uniform("fade_length_fraction", plight_fade_length_fraction);

    // Point light shadows.
    input_psm_->point_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(4);
    BindGuard bound_psm_sampler =                    psm_sampler_->bind_to_texture_unit(4);
    sp_singlepass_->uniform("point_shadow.maps",        4);
    sp_singlepass_->uniform("point_shadow.bias_bounds", point_params.bias_bounds);
    sp_singlepass_->uniform("point_shadow.pcf_extent",  point_params.pcf_extent);
    sp_singlepass_->uniform("point_shadow.pcf_offset",  point_params.pcf_offset);
    OffsetElems with_shadows_offset{ 0 };
    OffsetElems no_shadows_offset  { num_plights_with_shadow };
    if (num_plights_with_shadow != 0) {
        plights_buf_->bind_range_to_index<BufferTargetIndexed::ShaderStorage>(
            with_shadows_offset, num_plights_with_shadow, 1
        );
    } else {
        unbind_indexed_from_context<BindingIndexed::ShaderStorageBuffer>(1);
    }
    if (num_plights_no_shadow != 0) {
        plights_buf_->bind_range_to_index<BufferTargetIndexed::ShaderStorage>(
            no_shadows_offset,   num_plights_no_shadow,   2
        );
    } else {
        unbind_indexed_from_context<BindingIndexed::ShaderStorageBuffer>(2);
    }


    sp_singlepass_->uniform("cam_pos", engine.camera().transform.position());


    glapi::disable(Capability::DepthTesting);
    {
        BindGuard bound_program = sp_singlepass_->use();
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
    RenderEnginePrimaryInterface& engine,
    NumElems                      num_plights_with_shadow,
    NumElems                      num_plights_no_shadow)
{
    const auto& registry = engine.registry();

    gbuffer_->position_draw_texture().bind_to_texture_unit(0);
    gbuffer_->normals_texture()      .bind_to_texture_unit(1);
    gbuffer_->albedo_spec_texture()  .bind_to_texture_unit(2);
    MultibindGuard bound_gbuffer_samplers{
        target_sampler_             ->bind_to_texture_unit(0),
        target_sampler_             ->bind_to_texture_unit(1),
        target_sampler_             ->bind_to_texture_unit(2),
    };

    auto set_common_uniforms = [&](RawProgram<> sp) {
        sp.uniform("cam_pos", engine.camera().transform.position());
        sp.uniform("gbuffer.tex_position_draw", 0);
        sp.uniform("gbuffer.tex_normals",       1);
        sp.uniform("gbuffer.tex_albedo_spec",   2);
    };

    // Ambient + Directional Light Pass.
    {
        RawProgram<> sp = sp_pass_ambi_dir_;
        BindGuard bound_program = sp.use();

        set_common_uniforms(sp);

        // Ambient Light.
        auto& ambi = *registry.storage<light::Ambient>().begin();
        sp.uniform("ambi_light.color", ambi.color);

        // Ambient Occlusion.
        input_ao_->blurred_texture                   .bind_to_texture_unit(3);
        BindGuard bound_ao_sampler = target_sampler_->bind_to_texture_unit(3);
        sp.uniform("ambi_occlusion.tex_occlusion", 3);
        sp.uniform("ambi_occlusion.use",           use_ambient_occlusion);
        sp.uniform("ambi_occlusion.power",         ambient_occlusion_power);

        // Directional Light.
        auto [dlight_ent, dlight] = *registry.view<light::Directional>().each().begin();
        sp.uniform("dir_light.color",        dlight.color);
        sp.uniform("dir_light.direction",    dlight.direction);

        // CSM.
        input_csm_->dir_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(4);
        BindGuard bound_csm_sampler =                  csm_sampler_->bind_to_texture_unit(4);
        sp.uniform("dir_shadow.cascades",            4);
        sp.uniform("dir_shadow.do_cast",             registry.all_of<tags::ShadowCasting>(dlight_ent));
        sp.uniform("dir_shadow.base_bias_tx",        dir_params.base_bias_tx);
        sp.uniform("dir_shadow.do_blend_cascades",   dir_params.blend_cascades);
        sp.uniform("dir_shadow.blend_size_inner_tx", dir_params.blend_size_inner_tx);
        sp.uniform("dir_shadow.pcf_extent",          dir_params.pcf_extent);
        sp.uniform("dir_shadow.pcf_offset",          dir_params.pcf_offset);
        csm_params_buf_->bind_to_index<BufferTargetIndexed::ShaderStorage>(0);


        engine.draw([&](auto bound_fbo) {
            glapi::disable(Capability::DepthTesting);
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
            glapi::enable(Capability::DepthTesting);
        });
    }


    auto set_plight_uniforms = [&](RawProgram<> sp) {
        sp.uniform("view", engine.camera().view_mat());
        sp.uniform("proj", engine.camera().projection_mat());
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
    if (num_plights_no_shadow != 0) {
        RawProgram<> sp = sp_pass_plight_no_shadow_;
        BindGuard bound_program = sp.use();

        set_common_uniforms(sp);
        set_plight_uniforms(sp);

        OffsetElems no_shadows_offset{ num_plights_with_shadow };
        plights_buf_->bind_range_to_index<BufferTargetIndexed::ShaderStorage>(
            no_shadows_offset, num_plights_no_shadow, 0
        );


        instance_draw_plight_spheres(num_plights_no_shadow, bound_program.token());

    }

    // Point Lights With Shadows Pass.
    if (num_plights_with_shadow != 0) {
        RawProgram<> sp = sp_pass_plight_with_shadow_;
        BindGuard bound_program = sp.use();

        set_common_uniforms(sp);
        set_plight_uniforms(sp);

        OffsetElems with_shadows_offset{ 0 };
        plights_buf_->bind_range_to_index<BufferTargetIndexed::ShaderStorage>(
            with_shadows_offset, num_plights_with_shadow, 0
        );

        // Point Shadows.
        input_psm_->point_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(3);
        BindGuard bound_psm_sampler =                    psm_sampler_->bind_to_texture_unit(3);
        sp.uniform("point_shadows.maps",        3);
        sp.uniform("point_shadows.bias_bounds", point_params.bias_bounds);
        sp.uniform("point_shadows.pcf_extent",  point_params.pcf_extent);
        sp.uniform("point_shadows.pcf_offset",  point_params.pcf_offset);


        instance_draw_plight_spheres(num_plights_with_shadow, bound_program.token());

    }



}




void DeferredShading::update_cascade_buffer() {

    auto& params = input_csm_->params;
    resize_to_fit(csm_params_buf_, NumElems{ params.size() });

    std::span mapped = csm_params_buf_->map_for_write();
    do {
        std::ranges::copy(params, mapped.begin());
    } while (!csm_params_buf_->unmap_current());

}


auto DeferredShading::update_point_light_buffers(
    const entt::registry& registry)
        -> std::tuple<NumElems, NumElems>
{
    const size_t num_plights = registry.storage<light::Point>().size();

    NumElems num_with_shadow{ 0 };
    NumElems num_no_shadow  { 0 };

    if (num_plights == 0) {
        return { num_with_shadow, num_no_shadow };
    }


    resize_to_fit(plights_buf_, NumElems{ num_plights });

    auto plights_with_shadow_view = registry.view<light::Point, components::BoundingSphere, tags::ShadowCasting>();
    auto plights_no_shadow_view   = registry.view<light::Point, components::BoundingSphere>(entt::exclude<tags::ShadowCasting>);


    std::span mapped = plights_buf_->map_for_write();
    do {

        // From PointLight and BoundingSphere to combined PLight structure.
        auto repack = [](const auto& tuple) {
            const auto& [e, plight, sphere] = tuple;
            return PLight{
                .color       = plight.color,
                .position    = plight.position,
                .radius      = sphere.radius,
                .attenuation = plight.attenuation
            };
        };

        auto end_with_shadow = std::ranges::copy(
            plights_with_shadow_view.each() | std::views::transform(repack),
            mapped.begin()
        ).out;

        std::ranges::copy(
            plights_no_shadow_view.each() | std::views::transform(repack),
            end_with_shadow
        );

        num_with_shadow = NumElems(end_with_shadow - mapped.begin());
        num_no_shadow   = NumElems(num_plights - num_with_shadow);

    } while (!plights_buf_->unmap_current());


    return { num_with_shadow, num_no_shadow };
}


} // namespace josh::stages::primary
