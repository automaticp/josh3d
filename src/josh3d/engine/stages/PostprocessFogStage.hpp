#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include <entt/fwd.hpp>
#include <glm/matrix.hpp>



namespace josh {



/*
A fog effect with two variants:

    - Uniform fog density with a smoothstep towards full opacity
      close to the Z-far to mitigate Z-far-dependent issues.

      Will cover the entire screen, depth of 1 will be pure fog color.

    - Isothermal barometric fog, modelled as an ideal gas.
      Exponential decrease in density with height.

      For a point at infinity:
        - partially transparent in +Y hemisphere (can see the sky if not too deep),
        - fully opaque in the -Y hemisphere (not actually true due to Z-far effects).

I want to clarify the usage of the word "exponential" here.
It has nothing to do with the so-called "exponential fog" by itself,
the exponential decrease of direct transmittance over view distance
for uniform density is a given and is assumed default.
Everything else (linear, exponential sqared, etc.) is non-physical
and is not considered here as a base model.
*/
class PostprocessFogStage {
public:
    enum class FogType {
        none, uniform, barometric
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

private:
    ShaderProgram sp_uniform_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_fog_uniform.frag"))
            .get()
    };

    ShaderProgram sp_barometric_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_fog_barometric.frag"))
            .get()
    };

public:
    FogType   fog_type { FogType::none };
    glm::vec3 fog_color{ 1.f, 1.f, 1.f };

    UniformFogParams    uniform_fog_params{};
    BarometricFogParams barometric_fog_params{};


    void operator()(
        const RenderEnginePostprocessInterface& engine,
        const entt::registry&)
    {
        switch (fog_type) {
            using enum FogType;
            case none:       return;
            case uniform:    draw_uniform_fog(engine);    break;
            case barometric: draw_barometric_fog(engine); break;
        }
    }


private:
    void draw_uniform_fog(const RenderEnginePostprocessInterface& engine) {
        const auto& cam = engine.camera();

        const glm::mat4 inv_proj =
            glm::inverse(engine.camera().projection_mat());

        engine.screen_depth().bind_to_unit_index(1);

        sp_uniform_.use()
            .uniform("depth",     1)
            .uniform("fog_color", fog_color)
            .uniform("z_near",    cam.get_params().z_near)
            .uniform("z_far",     cam.get_params().z_far)
            .uniform("inv_proj",  inv_proj)
            .uniform("mean_free_path", uniform_fog_params.mean_free_path)
            .uniform("distance_power", uniform_fog_params.distance_power)
            .uniform("cutoff_offset",  uniform_fog_params.cutoff_offset)
            .and_then([&] {
                using namespace gl;

                // This postprocessing effect is a bit special in that it can
                // get by with just blending. So we blend directly with the
                // front buffer, skipping the swap.
                // There's no performance difference between swapping and not
                // if you blend with the whole screen, so whatever,
                // I do this for simplicity.

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                engine.draw_to_front();
                glDisable(GL_BLEND);

            });

    }


    void draw_barometric_fog(const RenderEnginePostprocessInterface& engine) {
        const auto& cam = engine.camera();

        const glm::mat4 inv_projview =
            glm::inverse(cam.projection_mat() * cam.view_mat());

        const auto  [H, Y0, L0]  = barometric_fog_params;
        const float base_density = glm::exp(Y0 / H) / L0;

        engine.screen_depth().bind_to_unit_index(1);

        sp_barometric_.use()
            .uniform("depth",        1)
            .uniform("fog_color",    fog_color)
            .uniform("cam_pos",      cam.transform.position())
            .uniform("z_near",       cam.get_params().z_near)
            .uniform("z_far",        cam.get_params().z_far)
            .uniform("inv_projview", inv_projview)
            .uniform("base_density", base_density)
            .uniform("scale_height", H)
            .and_then([&] {
                using namespace gl;
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                engine.draw_to_front();
                glDisable(GL_BLEND);
            });
    }
};


} // namespace josh

