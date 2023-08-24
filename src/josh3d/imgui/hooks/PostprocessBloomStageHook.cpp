#include "PostprocessBloomStageHook.hpp"
#include "ImGuiHelpers.hpp"
#include <imgui.h>



void josh::imguihooks::PostprocessBloomStageHook::operator()() {

    ImGui::Checkbox("Use Bloom", &stage_.use_bloom);

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

        ImGui::Unindent();

        const float w = ImGui::GetContentRegionAvail().x;
        const float h = w / stage_.blur_ppdb().size().aspect_ratio();

        ImGui::ImageGL(void_id(stage_.blur_ppdb().front_target().id()), { w, h });

        ImGui::Indent();

        ImGui::TreePop();
    }

}

