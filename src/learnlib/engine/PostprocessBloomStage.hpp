#pragma once
#include "GLObjects.hpp"
#include "PostprocessDoubleBuffer.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <imgui.h>



namespace learn {

class PostprocessBloomStage {
private:
    ShaderProgram sp_extract_{
        ShaderBuilder()
            .load_vert("src/shaders/postprocess.vert")
            .load_frag("src/shaders/pp_bloom_threshold_extract.frag")
            .get()
    };

    ShaderProgram sp_twopass_gaussian_blur_{
        ShaderBuilder()
            .load_vert("src/shaders/postprocess.vert")
            .load_frag("src/shaders/pp_bloom_twopass_gaussian_blur.frag")
            .get()
    };

    ShaderProgram sp_blend_{
        ShaderBuilder()
            .load_vert("src/shaders/postprocess.vert")
            .load_frag("src/shaders/pp_bloom_blend.frag")
            .get()
    };

    PostprocessDoubleBuffer blur_ppdb_{
        1024, 1024, gl::GL_RGBA, gl::GL_RGBA16F, gl::GL_FLOAT
    };


public:
    float threshold{ 1.f };
    size_t blur_iterations{ 2 };
    float offset_scale{ 1.f };


    void operator()(const RenderEngine::PostprocessInterface& engine, const entt::registry&) {
        using namespace gl;

        if (engine.window_size().width  != blur_ppdb_.back().width() ||
            engine.window_size().height != blur_ppdb_.back().height())
        {
            // TODO: Might be part of PPDB::reset_size() to skip redundant resets
            blur_ppdb_.reset_size(engine.window_size().width, engine.window_size().height);
        }

        // Extract
        blur_ppdb_.draw_and_swap([&, this] {
            sp_extract_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

                ashp.uniform("threshold", threshold)
                    .uniform("screen_color", 0);
                engine.screen_color().bind_to_unit(GL_TEXTURE0);

                engine.postprocess_renderer().draw();

            });
        });


        // Blur
        for (size_t i{ 0 }; i < (2 * blur_iterations); ++i) {
            blur_ppdb_.draw_and_swap([&, this] {
                sp_twopass_gaussian_blur_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

                    ashp.uniform("blur_horizontally", bool(i % 2))
                        .uniform("offset_scale", offset_scale)
                        .uniform("screen_color", 0);
                    blur_ppdb_.front_target().bind_to_unit(GL_TEXTURE0);

                    engine.postprocess_renderer().draw();
                });
            });
        }

        // Blend
        sp_blend_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform("screen_color", 0)
                .uniform("bloom_color", 1);
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            blur_ppdb_.front_target().bind_to_unit(GL_TEXTURE1);

            engine.draw();

        });

    }

};





class PostprocessBloomStageImGuiHook {
private:
    PostprocessBloomStage& stage_;
    int num_iterations_as_int_because_imgui_likes_ints_{
        static_cast<int>(stage_.blur_iterations)
    };

public:
    PostprocessBloomStageImGuiHook(PostprocessBloomStage& stage)
        : stage_{ stage }
    {}

    void operator()() {

        ImGui::SliderFloat(
            "Threshold", &stage_.threshold,
            0.f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Offset Scale", &stage_.offset_scale,
            0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        if (ImGui::SliderInt(
                "Num Iterations", &num_iterations_as_int_because_imgui_likes_ints_,
                1, 1024, "%d", ImGuiSliderFlags_Logarithmic
            ))
        {
            stage_.blur_iterations = num_iterations_as_int_because_imgui_likes_ints_;
        }

    }

};

} // namespace learn
