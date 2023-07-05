#include "ForwardRenderingStageHook.hpp"
#include <imgui.h>






void learn::imguihooks::ForwardRenderingStageHook::operator()() {

    auto& s = stage_;

    if (ImGui::TreeNode("Point Shadows")) {

        ImGui::SliderFloat2(
            "Shadow Bias", glm::value_ptr(s.point_params.bias_bounds),
            0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::Checkbox("Use Fixed PCF Samples", &s.point_params.use_fixed_pcf_samples);

        ImGui::BeginDisabled(s.point_params.use_fixed_pcf_samples);
        ImGui::SliderInt(
            "PCF Samples", &s.point_params.pcf_samples, 0, 6
        );
        ImGui::EndDisabled();

        ImGui::SliderFloat(
            "PCF Offset", &s.point_params.pcf_offset,
            0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Directional Shadows")) {

        ImGui::SliderFloat2(
            "Shadow Bias", glm::value_ptr(s.dir_params.bias_bounds),
            0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderInt(
            "PCF Samples", &s.dir_params.pcf_samples, 0, 12
        );

        ImGui::TreePop();
    }

}
