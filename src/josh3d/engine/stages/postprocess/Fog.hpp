#pragma once
#include "EnumUtils.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "UniformTraits.hpp"
#include "VPath.hpp"


namespace josh {


/*
A fog effect with two variants:

- Uniform fog density with a smoothstep towards full opacity
  close to the Z-far to mitigate Z-far-dependent issues.
  Will cover the entire screen, depth of 1 will be pure fog color.

- Isothermal barometric fog, modelled as an ideal gas.
    Exponential decrease in density with height.
    For a point at infinity:
    - Partially transparent in +Y hemisphere (can see the sky if not too deep),
    - Fully opaque in the -Y hemisphere.

*/
struct Fog
{
    enum class FogType
    {
        None,
        Uniform,
        Barometric,
    };

    struct UniformFogParams
    {
        float mean_free_path = 20.f;
        // Anything other than 1.0 is likely non-physical
        float distance_power = 1.f;
        // Offset distance from z-far that begins smoothstep
        // towards full fog opacity.
        float cutoff_offset = 0.5f;
    };

    struct BarometricFogParams
    {
        // Vertical fog density decay rate (H)
        float scale_height = 50.f;
        // Some height chosen for the scene (Y0)
        float base_height = 0.f;
        // Mean free path at base_height (L0)
        float base_mean_free_path = 20.f;
    };

    FogType fog_type  = FogType::None;
    vec3    fog_color = { 1.f, 1.f, 1.f };

    UniformFogParams    uniform_fog_params = {};
    BarometricFogParams barometric_fog_params = {};

    void operator()(RenderEnginePostprocessInterface& engine);

private:
    void draw_uniform_fog(RenderEnginePostprocessInterface& engine);
    void draw_barometric_fog(RenderEnginePostprocessInterface& engine);

    ShaderToken sp_uniform_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_fog_uniform.frag")});

    ShaderToken sp_barometric_= shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_fog_barometric.frag")});
};
JOSH3D_DEFINE_ENUM_EXTRAS(Fog::FogType, None, Uniform, Barometric);


} // namespace josh

