#include "DeferredShading.hpp"
#include "DefaultResources.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "LightCasters.hpp"
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

    draw_main(engine, num_plights_with_shadow, num_plights_no_shadow);
}


void DeferredShading::draw_main(
    RenderEnginePrimaryInterface& engine,
    NumElems                      num_plights_with_shadow, // TODO: Is there no better way to communicate this?
    NumElems                      num_plights_no_shadow)
{
    const auto& registry = engine.registry();


    // GBuffer.
    gbuffer_->position_draw_texture().bind_to_texture_unit(0);
    gbuffer_->normals_texture()      .bind_to_texture_unit(1);
    gbuffer_->albedo_spec_texture()  .bind_to_texture_unit(2);
    target_sampler_                 ->bind_to_texture_unit(0);
    target_sampler_                 ->bind_to_texture_unit(1);
    target_sampler_                 ->bind_to_texture_unit(2);
    sp_->uniform("tex_position_draw", 0);
    sp_->uniform("tex_normals",       1);
    sp_->uniform("tex_albedo_spec",   2);

    // AO.
    input_ao_->blurred_texture       .bind_to_texture_unit(5);
    target_sampler_                 ->bind_to_texture_unit(5);
    sp_->uniform("tex_ambient_occlusion",   5);
    sp_->uniform("use_ambient_occlusion",   use_ambient_occlusion);
    sp_->uniform("ambient_occlusion_power", ambient_occlusion_power);

    // Ambient light.
    auto& alight = *registry.storage<light::Ambient>().begin();
    sp_->uniform("ambient_light.color", alight.color);

    // Directional light.
    auto [dlight_ent, dlight] = *registry.view<light::Directional>().each().begin();
    sp_->uniform("dir_light.color",        dlight.color);
    sp_->uniform("dir_light.direction",    dlight.direction);
    sp_->uniform("dir_shadow.do_cast",     registry.all_of<tags::ShadowCasting>(dlight_ent));

    // Directional shadows.
    input_csm_->dir_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(3);
    csm_sampler_                                               ->bind_to_texture_unit(3);
    sp_->uniform("dir_shadow.cascades",            3);
    sp_->uniform("dir_shadow.base_bias_tx",        dir_params.base_bias_tx);
    sp_->uniform("dir_shadow.do_blend_cascades",   dir_params.blend_cascades);
    sp_->uniform("dir_shadow.blend_size_inner_tx", dir_params.blend_size_inner_tx);
    sp_->uniform("dir_shadow.pcf_extent",          dir_params.pcf_extent);
    sp_->uniform("dir_shadow.pcf_offset",          dir_params.pcf_offset);
    csm_params_buf_->bind_to_index<BufferTargetIndexed::ShaderStorage>(3);

    // Point light shadows.
    input_psm_->point_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(4);
    psm_sampler_                                                 ->bind_to_texture_unit(4);
    sp_->uniform("point_shadow.maps",        4);
    sp_->uniform("point_shadow.bias_bounds", point_params.bias_bounds);
    sp_->uniform("point_shadow.z_far",       input_psm_->z_near_far[1]);
    sp_->uniform("point_shadow.pcf_extent",  point_params.pcf_extent);
    sp_->uniform("point_shadow.pcf_offset",  point_params.pcf_offset);
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


    sp_->uniform("cam_pos", engine.camera().transform.position());


    glapi::disable(Capability::DepthTesting);

    auto bound_program = sp_->use();

    engine.draw([&](auto bound_fbo) {
        globals::quad_primitive_mesh().draw(bound_program, bound_fbo);
    });

    bound_program.unbind();

    glapi::enable(Capability::DepthTesting);

    // The depth buffer is probably shared between the GBuffer
    // and the main framebuffer.
    //
    // This is okay if the deferred shading algorithm does not depend
    // on the depth value. That is, if you need to isolate the
    // depth that was drawn only in deferred passes, then you might
    // have to do just that. And then do some kind of depth blending.

    unbind_samplers_from_units(0, 1, 2, 3, 4, 5);

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

    auto plights_with_shadow_view = registry.view<light::Point, tags::ShadowCasting>();
    auto plights_no_shadow_view   = registry.view<light::Point>(entt::exclude<tags::ShadowCasting>);


    std::span mapped = plights_buf_->map_for_write();
    do {

        auto end_with_shadow = std::ranges::copy(
            plights_with_shadow_view.each() | std::views::elements<1>,
            mapped.begin()
        ).out;

        std::ranges::copy(
            plights_no_shadow_view.each() | std::views::elements<1>,
            end_with_shadow
        );

        num_with_shadow = NumElems(end_with_shadow - mapped.begin());
        num_no_shadow   = NumElems(num_plights - num_with_shadow);

    } while (!plights_buf_->unmap_current());

    return { num_with_shadow, num_no_shadow };
}


} // namespace josh::stages::primary
