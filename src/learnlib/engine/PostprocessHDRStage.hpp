#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <imgui.h>



namespace learn {

class PostprocessHDRStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/postprocess.vert")
            .load_frag("src/shaders/pp_hdr.frag")
            .get()
    };

public:
    bool use_reinhard{ false };
    bool use_exposure{ true };
    float exposure{ 1.0f };

    void operator()(const RenderEngine::PostprocessInterface& engine, const entt::registry&) {
        using namespace gl;

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            ashp.uniform("color", 0)
                .uniform("use_reinhard", use_reinhard)
                .uniform("use_exposure", use_exposure)
                .uniform("exposure", exposure);

            engine.draw();
        });
    }

};



class PostprocessHDRStageImGuiHook {
private:
    PostprocessHDRStage& stage_;

public:
    PostprocessHDRStageImGuiHook(PostprocessHDRStage& stage)
        : stage_{ stage }
    {}

    void operator()() {
        ImGui::Checkbox("Use Reinhard", &stage_.use_reinhard);

        ImGui::BeginDisabled(stage_.use_reinhard);
        ImGui::Checkbox("Use Exposure", &stage_.use_exposure);
        ImGui::SliderFloat(
            "Exposure", &stage_.exposure,
            0.01f, 10.f, "%.2f", ImGuiSliderFlags_Logarithmic
        );
        ImGui::EndDisabled();
    }

};


} // namespace learn
