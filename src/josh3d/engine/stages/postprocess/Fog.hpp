#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "ShaderBuilder.hpp"
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
    UniqueProgram sp_uniform_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_fog_uniform.frag"))
            .get()
    };

    UniqueProgram sp_barometric_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_fog_barometric.frag"))
            .get()
    };


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

    const auto& cam = engine.camera();

    const glm::mat4 inv_proj = glm::inverse(engine.camera().projection_mat());

    engine.screen_depth().bind_to_texture_unit(1);
    sp_uniform_->uniform("depth",     1);
    sp_uniform_->uniform("fog_color", fog_color);
    sp_uniform_->uniform("z_near",    cam.get_params().z_near);
    sp_uniform_->uniform("z_far",     cam.get_params().z_far);
    sp_uniform_->uniform("inv_proj",  inv_proj);
    sp_uniform_->uniform("mean_free_path", uniform_fog_params.mean_free_path);
    sp_uniform_->uniform("distance_power", uniform_fog_params.distance_power);
    sp_uniform_->uniform("cutoff_offset",  uniform_fog_params.cutoff_offset);

    // This postprocessing effect is a bit special in that it can
    // get by with just blending. So we blend directly with the
    // front buffer, skipping the swap.
    // There's no performance difference between swapping and not
    // if you blend with the whole screen, so whatever,
    // I do this for simplicity.

    glapi::enable(Capability::Blending);
    glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    auto bound_program = sp_uniform_->use();
    engine.draw_to_front(bound_program);
    bound_program.unbind();

    glapi::disable(Capability::Blending);
}




inline void Fog::draw_barometric_fog(
    RenderEnginePostprocessInterface& engine)
{

    const auto& cam = engine.camera();

    const glm::mat4 inv_proj               = glm::inverse(cam.projection_mat());
    const glm::mat3 normal_view_mat        = glm::inverse(glm::transpose(cam.view_mat()));
    const glm::vec3 world_up_in_view_space = glm::normalize(normal_view_mat * globals::basis.y());

    // See comments in shader code to make sense of this.
    const auto  [H, Y0, L0]  = barometric_fog_params;
    const float base_density = glm::exp(Y0 / H) / L0;
    // We want to compute the effect in the view-space,
    // because world-space calculations incur precision issues
    // at large separation from the origin.
    const float eye_height = cam.transform.position().y;
    float density_at_eye_height = float(
        double(base_density) * glm::exp(-double(eye_height) / double(H))
    );


    engine.screen_depth().bind_to_texture_unit(1);
    sp_barometric_->uniform("depth",        1);
    sp_barometric_->uniform("fog_color",    fog_color);
    sp_barometric_->uniform("z_near",       cam.get_params().z_near);
    sp_barometric_->uniform("z_far",        cam.get_params().z_far);
    sp_barometric_->uniform("inv_proj",     inv_proj);
    sp_barometric_->uniform("scale_height", H);
    sp_barometric_->uniform("world_up_in_view_space", world_up_in_view_space);
    sp_barometric_->uniform("density_at_eye_height",  density_at_eye_height);

    glapi::enable(Capability::Blending);
    glapi::set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    auto bound_program = sp_barometric_->use();
    engine.draw_to_front(bound_program);
    bound_program.unbind();

    glapi::disable(Capability::Blending);
}




} // namespace josh::stages::postprocess

