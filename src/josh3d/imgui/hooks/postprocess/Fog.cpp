#include "Fog.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <cfloat>
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, Fog) {

    using FogType = stages::postprocess::Fog::FogType;

    ImGui::ColorEdit3(
        "Fog Color", glm::value_ptr(stage_.fog_color),
        ImGuiColorEditFlags_DisplayHSV
    );

    if (ImGui::RadioButton("Disabled", stage_.fog_type == FogType::None)) {
        stage_.fog_type = FogType::None;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Uniform", stage_.fog_type == FogType::Uniform)) {
        stage_.fog_type = FogType::Uniform;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Barometric", stage_.fog_type == FogType::Barometric)) {
        stage_.fog_type = FogType::Barometric;
    }


    if (stage_.fog_type == FogType::Uniform) {
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


    if (stage_.fog_type == FogType::Barometric) {
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

