#pragma once
#include "GLObjects.hpp"
#include "PostprocessDoubleBuffer.hpp"
#include "RenderEngine.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <imgui.h>
#include <cmath>
#include <range/v3/view/generate_n.hpp>
#include <cassert>
#include <cstddef>
#include <vector>



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

    SSBOWithIntermediateBuffer<float> weights_ssbo_{ 0 };
    float old_gaussian_sample_range_{ 1.8f };
    size_t old_gaussian_samples_{ 4 };

public:
    PostprocessBloomStage() {
        update_gaussian_blur_weights();
    }


    glm::vec2 threshold_bounds{ 0.05f, 1.0f };
    size_t blur_iterations{ 1 };
    float offset_scale{ 1.f };

    float gaussian_sample_range{ old_gaussian_sample_range_ };
    size_t gaussian_samples{ old_gaussian_samples_ };

    const Texture2D& blur_front_target() const noexcept {
        return blur_ppdb_.front_target();
    }

    // From -x to +x binned into 2 * n_samples + 1.
    void update_gaussian_blur_weights();
    bool gaussian_weights_need_updating() const noexcept {
        return gaussian_sample_range != old_gaussian_sample_range_ ||
            gaussian_samples != old_gaussian_samples_;
    }

    void operator()(const RenderEnginePostprocessInterface& engine, const entt::registry&) {
        using namespace gl;

        if (engine.window_size().width  != blur_ppdb_.back().width() ||
            engine.window_size().height != blur_ppdb_.back().height())
        {
            // TODO: Might be part of PPDB::reset_size() to skip redundant resets
            blur_ppdb_.reset_size(engine.window_size().width, engine.window_size().height);
        }

        if (gaussian_weights_need_updating()) {
            update_gaussian_blur_weights();
        }

        // Extract
        blur_ppdb_.draw_and_swap([&, this] {
            sp_extract_.use().and_then([&, this](ActiveShaderProgram& ashp) {

                ashp.uniform("threshold_bounds", threshold_bounds)
                    .uniform("screen_color", 0);
                engine.screen_color().bind_to_unit(GL_TEXTURE0);

                engine.postprocess_renderer().draw();

            });
        });

        // Blur
        weights_ssbo_.bind().and_then([&, this] {

            for (size_t i{ 0 }; i < (2 * blur_iterations); ++i) {
                blur_ppdb_.draw_and_swap([&, this] {
                    sp_twopass_gaussian_blur_.use().and_then([&, this](ActiveShaderProgram& ashp) {

                        ashp.uniform("blur_horizontally", bool(i % 2))
                            .uniform("offset_scale", offset_scale)
                            .uniform("screen_color", 0);
                        blur_ppdb_.front_target().bind_to_unit(GL_TEXTURE0);

                        engine.postprocess_renderer().draw();
                    });
                });
            }

        });

        // Blend
        sp_blend_.use().and_then([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform("screen_color", 0)
                .uniform("bloom_color", 1);
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            blur_ppdb_.front_target().bind_to_unit(GL_TEXTURE1);

            engine.draw();

        });

    }

    // Uniformly bins the normal distribution from x0 to x1.
    // Does not preserve the sum as the tails are not accounted for.
    // Accounting for tails can make them biased during sampling.
    // Does not normalize the resulting bins.
    static auto generate_binned_gaussian_no_tails(float from, float to,
        size_t n_bins) noexcept
    {
        assert(to > from);

        const float step{ (to - from) / float(n_bins) };
        float current_x{ from };
        float previous_cdf = gaussian_cdf(from);

        return ranges::views::generate_n(
            [=]() mutable {
                current_x += step;
                const float current_cdf{ gaussian_cdf(current_x) };
                const float diff = current_cdf - previous_cdf;
                previous_cdf = current_cdf;
                return diff;
            },
            n_bins
        );
    }

    static float gaussian_cdf(float x) noexcept {
        return (1.f + std::erf(x / std::sqrt(2.f))) / 2.f;
    }

};

inline void PostprocessBloomStage::update_gaussian_blur_weights()
{
    // FIXME: The weights are not normalized over the range of x
    // leading to a noticable loss of color yield when
    // the range is too high. Is this okay?
    weights_ssbo_.bind().update(
        generate_binned_gaussian_no_tails(
            -gaussian_sample_range, gaussian_sample_range,
            (gaussian_samples * 2) + 1
        )
    );
    old_gaussian_sample_range_ = gaussian_sample_range;
    old_gaussian_samples_ = gaussian_samples;
}




class PostprocessBloomStageImGuiHook {
private:
    PostprocessBloomStage& stage_;

public:
    PostprocessBloomStageImGuiHook(PostprocessBloomStage& stage)
        : stage_{ stage }
    {}

    void operator()() {

        ImGui::SliderFloat2(
            "Threshold", glm::value_ptr(stage_.threshold_bounds),
            0.f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Offset Scale", &stage_.offset_scale,
            0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );


        auto num_iterations = static_cast<int>(stage_.blur_iterations);
        if (ImGui::SliderInt(
                "Num Iterations", &num_iterations,
                1, 128, "%d", ImGuiSliderFlags_Logarithmic
            ))
        {
            stage_.blur_iterations = num_iterations;
        }

        if (ImGui::TreeNode("Gaussian Blur")) {

            ImGui::DragFloat(
                "Range [-x, +x]", &stage_.gaussian_sample_range,
                0.1f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic
            );

            auto num_samples = static_cast<int>(stage_.gaussian_samples);
            if (ImGui::SliderInt(
                    "Num Samples", &num_samples,
                    0, 15, "%d", ImGuiSliderFlags_Logarithmic
                ))
            {
                stage_.gaussian_samples = num_samples;
            }

            ImGui::TreePop();
        }


        if (ImGui::TreeNode("Bloom Texture")) {

            ImGui::Image(
                reinterpret_cast<ImTextureID>(
                    stage_.blur_front_target().id()
                ),
                { 300, 300 }
            );

            ImGui::TreePop();
        }

    }

};

} // namespace learn
