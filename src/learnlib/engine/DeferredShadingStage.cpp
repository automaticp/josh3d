#include "DeferredShadingStage.hpp"
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include <entt/entity/registry.hpp>
#include <glbinding/gl/gl.h>
#include <range/v3/all.hpp>


using namespace gl;

namespace learn {



void DeferredShadingStage::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry& registry)
{

    update_point_light_buffers(registry);


    sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

        gbuffer_->position_target().bind_to_unit_index(0);
        gbuffer_->normals_target().bind_to_unit_index(1);
        gbuffer_->albedo_spec_target().bind_to_unit_index(2);

        ashp.uniform("tex_position_draw", 0)
            .uniform("tex_normals", 1)
            .uniform("tex_albedo_spec", 2);

        for (auto [_, ambi]
            : registry.view<light::Ambient>().each())
        {
            ashp.uniform("ambient_light.color", ambi.color);
        }

        for (auto [e, dir]
            : registry.view<light::Directional>().each())
        {
            ashp.uniform("dir_light.color",        dir.color)
                .uniform("dir_light.direction",    dir.direction);
        }

        ashp.uniform("cam_pos", engine.camera().get_pos());


        engine.draw([&, this] {
            glDisable(GL_DEPTH_TEST);
            quad_renderer_.draw();
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




void DeferredShadingStage::update_point_light_buffers(
    const entt::registry& registry)
{
    auto plights_view = registry.view<light::Point>();

    plights_ssbo_.bind().update(
        plights_view | ranges::views::transform([&](entt::entity e) {
            return plights_view.get<light::Point>(e);
        })
    );

}




} // namespace learn
