#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <imgui.h>




namespace learn {

class PostprocessGammaCorrectionStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/postprocess.vert")
            .load_frag("src/shaders/pp_gamma.frag")
            .get()
    };

public:
    float gamma{ 2.2f };
    bool use_srgb{ true };

    void operator()(const RenderEnginePostprocessInterface& engine, const entt::registry&) {

        using namespace gl;

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            ashp.uniform("color", 0);

            if (use_srgb) {
                ashp.uniform("gamma", 1.0f);
                glEnable(GL_FRAMEBUFFER_SRGB);
                engine.draw();
                glDisable(GL_FRAMEBUFFER_SRGB);
            } else /* custom gamma */ {
                ashp.uniform("gamma", gamma);
                engine.draw();
            }

        });


    }

};



class PostprocessGammaCorrectionStageImGuiHook {
private:
    PostprocessGammaCorrectionStage& stage_;

public:
    PostprocessGammaCorrectionStageImGuiHook(PostprocessGammaCorrectionStage& stage)
        : stage_{ stage }
    {}

    void operator()() {
        ImGui::Checkbox("Use sRGB", &stage_.use_srgb);

        ImGui::BeginDisabled(stage_.use_srgb);
        ImGui::SliderFloat(
            "Gamma", &stage_.gamma,
            0.0f, 10.f, "%.1f"
        );
        ImGui::EndDisabled();
    }
};




} // namespace learn
