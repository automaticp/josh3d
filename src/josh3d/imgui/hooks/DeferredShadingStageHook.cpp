#include "DeferredShadingStageHook.hpp"
#include <imgui.h>



void josh::imguihooks::DeferredShadingStageHook::operator()() {


    if (ImGui::TreeNode("Point Shadows")) {

        ImGui::SliderFloat2(
            "Shadow Bias",
            glm::value_ptr(stage_.point_params.bias_bounds),
            0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderInt(
            "PCF Samples", &stage_.point_params.pcf_samples, 0, 6
        );

        ImGui::SliderFloat(
            "PCF Offset", &stage_.point_params.pcf_offset,
            0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Directional Shadows")) {

        ImGui::SliderFloat2(
            "Shadow Bias",
            glm::value_ptr(stage_.dir_params.bias_bounds),
            0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderInt(
            "PCF Samples", &stage_.dir_params.pcf_samples, 0, 12
        );

        ImGui::SliderFloat(
            "PCF Offset", &stage_.dir_params.pcf_offset,
            0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


}
