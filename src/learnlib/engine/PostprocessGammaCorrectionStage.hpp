#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include <entt/entt.hpp>
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

    void operator()(const RenderEngine::PostprocessInterface& engine, const entt::registry&) {

        using namespace gl;

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            ashp.uniform("color", 0)
                .uniform("gamma", gamma);

            engine.draw();
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
        ImGui::SliderFloat(
            "Gamma", &stage_.gamma,
            0.0f, 10.f, "%.1f"
        );
    }
};




} // namespace learn
