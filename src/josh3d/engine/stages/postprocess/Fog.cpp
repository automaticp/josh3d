#include "Fog.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "UniformTraits.hpp"


namespace josh {


void Fog::operator()(
    RenderEnginePostprocessInterface& engine)
{
    switch (fog_type)
    {
        using enum FogType;
        case None:       return;
        case Uniform:    draw_uniform_fog(engine);    break;
        case Barometric: draw_barometric_fog(engine); break;
    }
}

void Fog::draw_uniform_fog(
    RenderEnginePostprocessInterface& engine)
{
    const auto sp = sp_uniform_.get();
    const BindGuard bcam = engine.bind_camera_ubo();

    engine.screen_depth().bind_to_texture_unit(1);
    sp.uniform("depth",          1);
    sp.uniform("fog_color",      fog_color);
    sp.uniform("mean_free_path", uniform_fog_params.mean_free_path);
    sp.uniform("distance_power", uniform_fog_params.distance_power);
    sp.uniform("cutoff_offset",  uniform_fog_params.cutoff_offset);

    const BindGuard bsp = sp.use();

    // This postprocessing effect is a bit special in that it can
    // get by with just blending. So we blend directly with the
    // front buffer, skipping the swap.
    // There's no performance difference between swapping and not
    // if you blend with the whole screen, so whatever,
    // I do this for simplicity.

    glapi::enable(Capability::Blending);
    glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    engine.draw_to_front(bsp);

    glapi::disable(Capability::Blending);
}

void Fog::draw_barometric_fog(
    RenderEnginePostprocessInterface& engine)
{
    const auto sp = sp_barometric_.get();
    const BindGuard bcam = engine.bind_camera_ubo();

    // See comments in shader code to make sense of this.
    const auto  [H, Y0, L0]  = barometric_fog_params;
    const float base_density = glm::exp(Y0 / H) / L0;

    // NOTE: We want to compute the effect in the view-space,
    // because world-space calculations incur precision issues
    // at large separation from the origin.
    const float eye_height = engine.camera_data().position_ws.y;
    const float density_at_eye_height =
        float(double(base_density) * glm::exp(-double(eye_height) / double(H)));

    engine.screen_depth().bind_to_texture_unit(1);
    sp.uniform("depth",                  1);
    sp.uniform("fog_color",              fog_color);
    sp.uniform("scale_height",           H);
    sp.uniform("density_at_eye_height",  density_at_eye_height);

    const BindGuard bsp = sp.use();

    glapi::enable(Capability::Blending);
    glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    engine.draw_to_front(bsp);

    glapi::disable(Capability::Blending);
}


} // namespace josh
