#include "CSMDebug.hpp"
#include "GLAPIBinding.hpp"
#include "Active.hpp"
#include "LightCasters.hpp"
#include "Transform.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include <ranges>


namespace josh::stages::overlay {


void CSMDebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    switch (mode)
    {
        case OverlayMode::Views: return draw_views_overlay(engine);
        case OverlayMode::Maps:  return draw_maps_overlay(engine);
        case OverlayMode::None:  return;
    }
}


void CSMDebug::draw_views_overlay(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();
    const auto* gbuffer  = engine.belt().try_get<GBuffer>();
    const auto* cascades = engine.belt().try_get<Cascades>();

    if (const auto dlight = get_active<DirectionalLight, Transform>(registry)) {

        const glm::vec3 light_dir = dlight.get<Transform>().orientation() * glm::vec3{ 0.f, 0.f, -1.f };

        const auto sp = sp_views_.get();
        BindGuard bound_camera = engine.bind_camera_ubo();

        csm_views_buf_.restage(cascades->views | transform(CascadeViewGPU::create_from));
        csm_views_buf_.bind_to_ssbo_index(3);

        gbuffer->depth_texture()  .bind_to_texture_unit(0);
        gbuffer->normals_texture().bind_to_texture_unit(1);

        sp.uniform("tex_depth",   0);
        sp.uniform("tex_normals", 1);

        sp.uniform("dir_light.color",     dlight.get<DirectionalLight>().hdr_color());
        sp.uniform("dir_light.direction", light_dir);

        glapi::disable(Capability::DepthTesting);
        {
            BindGuard bound_program = sp.use();
            engine.draw_fullscreen_quad(bound_program);
        }
        glapi::enable(Capability::DepthTesting);

    }
}


void CSMDebug::draw_maps_overlay(
    RenderEngineOverlayInterface& engine)
{
    const auto* cascades = engine.belt().try_get<Cascades>();

    if (not cascades) return;

    const auto sp = sp_maps_.get();
    cascades->maps.depth_attachment().texture() .bind_to_texture_unit(0);
    BindGuard bound_maps_sampler = maps_sampler_->bind_to_texture_unit(0);

    sp.uniform("cascades",   0);
    sp.uniform("cascade_id", cascade_id);

    glapi::disable(Capability::DepthTesting);
    {
        BindGuard bound_program = sp.use();
        engine.draw_fullscreen_quad(bound_program);
    }
    glapi::enable(Capability::DepthTesting);
}



} // namespace josh::stages::overlay
