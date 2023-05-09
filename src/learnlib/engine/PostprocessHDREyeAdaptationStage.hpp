#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <imgui.h>
#include <numeric>



namespace learn {

class PostprocessHDREyeAdaptationStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/postprocess.vert")
            .load_frag("src/shaders/pp_hdr.frag")
            .get()
    };

    ShaderProgram reduce_sp_{
        ShaderBuilder()
            .load_comp("src/shaders/pp_hdr_eye_adaptation_screen_reduce.comp")
            .get()
    };

    SSBOWithIntermediateBuffer<float> reduced_ssbo_{ 0, gl::GL_DYNAMIC_READ };
    size_t old_num_samples_{ 64 };

public:
    float current_screen_value{ 1.0f };
    float exposure_factor{ 1.f };
    float adaptation_rate{ 10.f };
    size_t num_samples{ old_num_samples_ };
    bool use_adaptation{ true };

    PostprocessHDREyeAdaptationStage() noexcept {
        resize_output_storage();
    }

    void operator()(const RenderEngine::PostprocessInterface& engine, const entt::registry&) {
        using namespace gl;

        if (use_adaptation) {

            const float avg_screen_value = // this frame
                compute_avg_screen_value(engine);

            const float frame_weight =
                engine.frame_timer().delta<float>();

            current_screen_value =
                scaled_weighted_mean_fold(current_screen_value,
                    avg_screen_value, frame_weight, adaptation_rate);
        }

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            ashp.uniform("color", 0)
                .uniform("use_reinhard", false)
                .uniform("use_exposure", true)
                .uniform("exposure", exposure_function(current_screen_value));

            engine.draw();
        });
    }



private:
    static float scaled_weighted_mean_fold(float current_mean,
        float value, float weight, float scale) noexcept
    {
        // Actually no clue what this does exactly.
        // I needed something similar to a weighted running mean,
        // except I only have the current average and an incoming value,
        // and NO context about any previous values used in calculating the mean.
        //
        // The actual plot of mean vs time for a sudden change
        // of a value looks a lot like the Integrator Circuit's
        // charge/discharge response to a step pulse.
        // So, uhhh, if you're into that kind of thing...
        //
        // The adaptation_rate (passed as scale) is actually pretty consistent
        // across jittery and inconsistent frametimes so that's good.
        //
        // It also looks okay when rendering so I'm not complaining.
        return (current_mean + scale * weight * value) /
            (1 + scale * weight);
    }


    float exposure_function(float screen_value) const noexcept {
        return exposure_factor / (screen_value + 0.0001f);
    }


    float compute_avg_screen_value(const RenderEngine::PostprocessInterface& engine) {
        using namespace gl;

        if (needs_storage_resize()) {
            resize_output_storage();
        }

        reduce_sp_.use()
            .and_then_with_self([&, this](ActiveShaderProgram& ashp) {

                engine.screen_color().bind_to_unit(GL_TEXTURE0);
                ashp.uniform("screen_color", 0);

                reduced_ssbo_.bind()
                    .and_then([this] {
                        glDispatchCompute(num_samples, num_samples, 1);
                        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                    })
                    .read_to_storage();

            });

        // Do the rest of the reduction on CPU.
        const auto& storage = reduced_ssbo_.storage();

        const float screen_value =
            std::reduce(storage.begin(), storage.end()) / float(storage.size());

        return screen_value;
    }

private:
    bool needs_storage_resize() {
        return old_num_samples_ != num_samples;
    }

    void resize_output_storage() {
        reduced_ssbo_.bind().create_storage(num_samples * num_samples);
        old_num_samples_ = num_samples;
    }

};



class PostprocessHDREyeAdaptationStageImGuiHook {
private:
    PostprocessHDREyeAdaptationStage& stage_;

public:
    PostprocessHDREyeAdaptationStageImGuiHook(
        PostprocessHDREyeAdaptationStage& stage)
        : stage_{ stage }
    {}


    void operator()() {

        ImGui::Checkbox("Use Adaptation", &stage_.use_adaptation);
        ImGui::DragFloat(
            "Adaptation Rate", &stage_.adaptation_rate,
            1.f, 1.f, 1080.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );
        // ImGui::DragFloat(
        //     "Exposure", &stage_.current_exposure,
        //     0.5f, 0.0f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
        // );
        ImGui::DragFloat(
            "Screen Value", &stage_.current_screen_value,
            0.5f, 0.0f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );
        ImGui::DragFloat(
            "Exposure Factor", &stage_.exposure_factor,
            0.5f, 0.0, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );
        int num_samples = static_cast<int>(stage_.num_samples);
        if (ImGui::SliderInt(
            "Num Samples", &num_samples,
            1, 1024, "%d", ImGuiSliderFlags_Logarithmic))
        {
            stage_.num_samples = num_samples;
        }

    }


};

} // namespace learn
