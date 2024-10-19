#pragma once
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "UniformTraits.hpp"
#include "VPath.hpp"
#include <entt/fwd.hpp>
#include <glm/matrix.hpp>




namespace josh::stages::postprocess {


/*
A fog effect with two variants:

    - Uniform fog density with a smoothstep towards full opacity
      close to the Z-far to mitigate Z-far-dependent issues.

      Will cover the entire screen, depth of 1 will be pure fog color.

    - Isothermal barometric fog, modelled as an ideal gas.
      Exponential decrease in density with height.

      For a point at infinity:
        - partially transparent in +Y hemisphere (can see the sky if not too deep),
        - fully opaque in the -Y hemisphere.

I want to clarify the usage of the word "exponential" here.
("I'd just like to interject for a moment..." moment)
It has nothing to do with the so-called "exponential fog" by itself,
the exponential decrease of direct transmittance over view distance
for uniform density is a given and is assumed default.
Everything else (linear, exponential sqared, etc.) is non-physical
and is not considered here as a base model.
*/
class Fog {
public:
    enum class FogType {
        None, Uniform, Barometric
    };

    struct UniformFogParams {
        float mean_free_path{ 20.f };
        // Anything other than 1.0 is likely non-physical
        float distance_power{ 1.f  };
        // Offset distance from z-far that begins smoothstep
        // towards full fog opacity.
        float cutoff_offset{ 0.5f };
    };

    struct BarometricFogParams {
        // Vertical fog density decay rate (H)
        float scale_height{ 50.f };
        // Some height chosen for the scene (Y0)
        float base_height { 0.f  };
        // Mean free path at base_height (L0)
        float base_mean_free_path{ 20.f };
    };

    FogType   fog_type { FogType::None };
    glm::vec3 fog_color{ 1.f, 1.f, 1.f };

    UniformFogParams    uniform_fog_params{};
    BarometricFogParams barometric_fog_params{};

    void operator()(RenderEnginePostprocessInterface& engine);



private:
    ShaderToken sp_uniform_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_fog_uniform.frag")});

    ShaderToken sp_barometric_= shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_fog_barometric.frag")});

    void draw_uniform_fog(RenderEnginePostprocessInterface& engine);
    void draw_barometric_fog(RenderEnginePostprocessInterface& engine);

};




inline void Fog::operator()(
    RenderEnginePostprocessInterface& engine)
{
    switch (fog_type) {
        using enum FogType;
        case None:       return;
        case Uniform:    draw_uniform_fog(engine);    break;
        case Barometric: draw_barometric_fog(engine); break;
    }
}




inline void Fog::draw_uniform_fog(
    RenderEnginePostprocessInterface& engine)
{
    const auto sp = sp_uniform_.get();
    BindGuard bound_camera_ubo = engine.bind_camera_ubo();

    engine.screen_depth().bind_to_texture_unit(1);
    sp.uniform("depth",          1);
    sp.uniform("fog_color",      fog_color);
    sp.uniform("mean_free_path", uniform_fog_params.mean_free_path);
    sp.uniform("distance_power", uniform_fog_params.distance_power);
    sp.uniform("cutoff_offset",  uniform_fog_params.cutoff_offset);

    // This postprocessing effect is a bit special in that it can
    // get by with just blending. So we blend directly with the
    // front buffer, skipping the swap.
    // There's no performance difference between swapping and not
    // if you blend with the whole screen, so whatever,
    // I do this for simplicity.

    glapi::enable(Capability::Blending);
    glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    {
        BindGuard bound_program = sp.use();
        engine.draw_to_front(bound_program);
    }
    glapi::disable(Capability::Blending);
}




inline void Fog::draw_barometric_fog(
    RenderEnginePostprocessInterface& engine)
{
    const auto sp = sp_barometric_.get();
    BindGuard bound_camera_ubo = engine.bind_camera_ubo();

    // See comments in shader code to make sense of this.
    const auto  [H, Y0, L0]  = barometric_fog_params;
    const float base_density = glm::exp(Y0 / H) / L0;
    // We want to compute the effect in the view-space,
    // because world-space calculations incur precision issues
    // at large separation from the origin.
    const float eye_height = engine.camera_data().position_ws.y;
    float density_at_eye_height = float(
        double(base_density) * glm::exp(-double(eye_height) / double(H))
    );


    engine.screen_depth().bind_to_texture_unit(1);
    sp.uniform("depth",                  1);
    sp.uniform("fog_color",              fog_color);
    sp.uniform("scale_height",           H);
    sp.uniform("density_at_eye_height",  density_at_eye_height);

    glapi::enable(Capability::Blending);
    glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    {
        BindGuard bound_program = sp.use();
        engine.draw_to_front(bound_program);
    }
    glapi::disable(Capability::Blending);
}




} // namespace josh::stages::postprocess

