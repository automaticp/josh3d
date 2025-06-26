#include "CSMDebug.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "Active.hpp"
#include "Geometry.hpp"
#include "LightCasters.hpp"
#include "Ranges.hpp"
#include "Transform.hpp"
#include "Tracy.hpp"
#include "stages/primary/GBufferStorage.hpp"


namespace josh {


void CSMDebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    ZSCGPUN("CSMDebug");
    switch (mode)
    {
        case OverlayMode::None:  return;
        case OverlayMode::Views: return draw_views_overlay(engine);
        case OverlayMode::Maps:  return draw_maps_overlay(engine);
    }
}

void CSMDebug::draw_views_overlay(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();
    const auto* gbuffer  = engine.belt().try_get<GBuffer>();
    const auto* cascades = engine.belt().try_get<Cascades>();

    if (const CHandle dlight = get_active<DirectionalLight, MTransform>(registry))
    {
        const vec3 light_dir = decompose_rotation(dlight.get<MTransform>()) * -Z;

        const auto sp = sp_views_.get();
        const BindGuard bcam = engine.bind_camera_ubo();

        csm_views_buf_.restage(cascades->views | transform(CascadeViewGPU::create_from));
        csm_views_buf_.bind_to_ssbo_index(3);

        gbuffer->depth_texture()  .bind_to_texture_unit(0);
        gbuffer->normals_texture().bind_to_texture_unit(1);

        sp.uniform("tex_depth",   0);
        sp.uniform("tex_normals", 1);

        sp.uniform("dir_light.color",     dlight.get<DirectionalLight>().hdr_color());
        sp.uniform("dir_light.direction", light_dir);

        const BindGuard bsp = sp.use();

        glapi::disable(Capability::DepthTesting);
        engine.draw_fullscreen_quad(bsp);
        glapi::enable(Capability::DepthTesting);
    }
}

void CSMDebug::draw_maps_overlay(
    RenderEngineOverlayInterface& engine)
{
    const auto* cascades = engine.belt().try_get<Cascades>();

    if (not cascades) return;

    _update_cascade_info(*cascades);
    const u32 cascade_idx = current_cascade_idx();

    const auto sp = sp_maps_.get();
    cascades->maps.textures().bind_to_texture_unit(0);
    const BindGuard bound_sampler = maps_sampler_->bind_to_texture_unit(0);

    sp.uniform("cascades",   0);
    sp.uniform("cascade_id", cascade_idx);

    const BindGuard bsp = sp.use();

    glapi::disable(Capability::DepthTesting);
    engine.draw_fullscreen_quad(bsp);
    glapi::enable(Capability::DepthTesting);
}

void CSMDebug::_update_cascade_info(const Cascades& cascades)
{
    const usize num_cascades = cascades.views.size();
    last_cascade_idx_    = std::clamp(desired_cascade_idx_, uindex(0), num_cascades - 1);
    desired_cascade_idx_ = last_cascade_idx_;
    last_num_cascades_   = num_cascades;
}

} // namespace josh
