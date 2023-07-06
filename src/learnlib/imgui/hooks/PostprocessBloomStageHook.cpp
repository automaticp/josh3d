#include "PostprocessBloomStageHook.hpp"
#include "ImGuiHelpers.hpp"
#include <imgui.h>



void learn::imguihooks::PostprocessBloomStageHook::operator()() {

    ImGui::SliderFloat2(
        "Threshold", glm::value_ptr(stage_.threshold_bounds),
        0.f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::SliderFloat(
        "Offset Scale", &stage_.offset_scale,
        0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );


    auto num_iterations = static_cast<int>(stage_.blur_iterations);
    if (ImGui::SliderInt("Num Iterations", &num_iterations,
        1, 128, "%d", ImGuiSliderFlags_Logarithmic))
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

        ImGui::ImageGL(void_id(stage_.blur_front_target().id()), { 300.f, 300.f });

        ImGui::TreePop();
    }

}

