#include "PostprocessFogStageHook.hpp"
#include <cfloat>
#include <imgui.h>



namespace josh::imguihooks {




void PostprocessFogStageHook::operator()() {
    using FogType = PostprocessFogStage::FogType;

    ImGui::ColorEdit3(
        "Fog Color", glm::value_ptr(stage_.fog_color),
        ImGuiColorEditFlags_DisplayHSV
    );

    if (ImGui::RadioButton("Disabled", stage_.fog_type == FogType::none)) {
        stage_.fog_type = FogType::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Uniform", stage_.fog_type == FogType::uniform)) {
        stage_.fog_type = FogType::uniform;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Barometric", stage_.fog_type == FogType::barometric)) {
        stage_.fog_type = FogType::barometric;
    }


    if (stage_.fog_type == FogType::uniform) {
        auto& params = stage_.uniform_fog_params;

        ImGui::DragFloat(
            "Mean Free Path", &params.mean_free_path,
            1.f, 0.1f, 1.e4f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::DragFloat(
            "Distance Power", &params.distance_power,
            0.025f, -16.f, 16.f
        );

        ImGui::DragFloat(
            "Z-far Cutoff", &params.cutoff_offset,
            0.1f, 0.01f, 1.e2f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }


    if (stage_.fog_type == FogType::barometric) {
        auto& params = stage_.barometric_fog_params;

        ImGui::DragFloat(
            "Scale Height", &params.scale_height,
            1.f, 0.1f, 1.e4f, "%.1f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::DragFloat(
            "Base Height", &params.base_height,
            1.f, -FLT_MAX, FLT_MAX, "%.3f"
        );

        ImGui::DragFloat(
            "MFP at Base Height", &params.base_mean_free_path,
            1.f, 0.1f, 1.e4f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }


}









} // namespace josh::imguihooks
