#include "DeferredShading.hpp"
#include "DefaultResources.hpp"
#include "GLShaders.hpp"
#include "LightCasters.hpp"
#include "tags/ShadowCasting.hpp"
#include "RenderEngine.hpp"
#include <entt/entity/registry.hpp>
#include <glbinding/gl/gl.h>
#include <range/v3/all.hpp>


using namespace gl;


namespace josh::stages::primary {


void DeferredShading::operator()(
    RenderEnginePrimaryInterface& engine)
{

    update_point_light_buffers(engine.registry());
    update_cascade_buffer();

    draw_main(engine);

}


void DeferredShading::draw_main(
    RenderEnginePrimaryInterface& engine)
{
    const auto& registry = engine.registry();

    sp_.use().and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {

        gbuffer_->position_draw_texture().bind_to_unit_index(0);
        gbuffer_->normals_texture()      .bind_to_unit_index(1);
        gbuffer_->albedo_spec_texture()  .bind_to_unit_index(2);
        ambient_occlusion_               .bind_to_unit_index(5);

        ashp.uniform("tex_position_draw",     0)
            .uniform("tex_normals",           1)
            .uniform("tex_albedo_spec",       2);

        ashp.uniform("use_ambient_occlusion",   use_ambient_occlusion)
            .uniform("ambient_occlusion_power", ambient_occlusion_power)
            .uniform("tex_ambient_occlusion",   5);

        for (auto [_, ambi]
            : registry.view<light::Ambient>().each())
        {
            ashp.uniform("ambient_light.color", ambi.color);
        }

        for (auto [e, dir]
            : registry.view<light::Directional>().each())
        {
            ashp.uniform("dir_light.color",        dir.color)
                .uniform("dir_light.direction",    dir.direction)
                .uniform("dir_shadow.do_cast",     registry.all_of<tags::ShadowCasting>(e));
        }


        input_csm_->dir_shadow_maps_tgt.depth_attachment().texture().bind_to_unit_index(3);
        ashp.uniform("dir_shadow.cascades", 3)
            .uniform("dir_shadow.base_bias_tx",        dir_params.base_bias_tx)
            .uniform("dir_shadow.do_blend_cascades",   dir_params.blend_cascades)
            .uniform("dir_shadow.blend_size_inner_tx", dir_params.blend_size_inner_tx)
            .uniform("dir_shadow.pcf_extent",          dir_params.pcf_extent)
            .uniform("dir_shadow.pcf_offset",          dir_params.pcf_offset);


        input_psm_->point_shadow_maps_tgt.depth_attachment().texture().bind_to_unit_index(4);
        ashp.uniform("point_shadow.maps", 4)
            .uniform("point_shadow.bias_bounds", point_params.bias_bounds)
            .uniform("point_shadow.z_far",       input_psm_->z_near_far[1])
            .uniform("point_shadow.pcf_extent",  point_params.pcf_extent)
            .uniform("point_shadow.pcf_offset",  point_params.pcf_offset);


        ashp.uniform("cam_pos", engine.camera().transform.position());


        engine.draw([] {
            glDisable(GL_DEPTH_TEST);
            globals::quad_primitive_mesh().draw();
            glEnable(GL_DEPTH_TEST);
        });

        // The depth buffer is probably shared between the GBuffer
        // and the main framebuffer.
        //
        // This is okay if the deferred shading algorithm does not depend
        // on the depth value. That is, if you need to isolate the
        // depth that was drawn only in deferred passes, then you might
        // have to do just that. And then do some kind of depth blending.

    });

}




void DeferredShading::update_cascade_buffer() {
    cascade_params_ssbo_.bind().update(input_csm_->params);
}


void DeferredShading::update_point_light_buffers(
    const entt::registry& registry)
{

    auto plights_with_shadow_view =
        registry.view<light::Point, tags::ShadowCasting>();

    plights_with_shadows_ssbo_.bind().update(
        plights_with_shadow_view | ranges::views::transform([&](entt::entity e) {
            return plights_with_shadow_view.get<light::Point>(e);
        })
    );

    auto plights_no_shadow_view =
        registry.view<light::Point>(entt::exclude<tags::ShadowCasting>);

    plights_no_shadows_ssbo_.bind().update(
        plights_no_shadow_view | ranges::views::transform([&](entt::entity e) {
            return plights_with_shadow_view.get<light::Point>(e);
        })
    );

}


} // namespace josh::stages::primary
