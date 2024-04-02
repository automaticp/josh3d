#include "Sky.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/primary/Sky.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, Sky) {

    using SkyType = stages::primary::Sky::SkyType;

    if (ImGui::RadioButton("None", stage_.sky_type == SkyType::None)) {
        stage_.sky_type = SkyType::None;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Debug", stage_.sky_type == SkyType::Debug)) {
        stage_.sky_type = SkyType::Debug;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Skybox", stage_.sky_type == SkyType::Skybox)) {
        stage_.sky_type = SkyType::Skybox;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Procedural", stage_.sky_type == SkyType::Procedural)) {
        stage_.sky_type = SkyType::Procedural;
    }

    if (stage_.sky_type == SkyType::Procedural) {

        auto& params = stage_.procedural_sky_params;

        ImGui::ColorEdit3(
            "Sky Color", glm::value_ptr(params.sky_color),
            ImGuiColorEditFlags_DisplayHSV
        );

        ImGui::ColorEdit3(
            "Sun Color", glm::value_ptr(params.sun_color),
            ImGuiColorEditFlags_DisplayHSV
        );

        ImGui::SliderFloat(
            "Sun Diameter, deg", &params.sun_size_deg,
            0.f, 45.f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }

}
