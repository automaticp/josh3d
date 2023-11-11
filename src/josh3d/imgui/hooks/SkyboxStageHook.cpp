#include "SkyboxStageHook.hpp"
#include "stages/SkyboxStage.hpp"
#include <imgui.h>


namespace josh::imguihooks {


void SkyboxStageHook::operator()() {
    using SkyType = SkyboxStage::SkyType;

    if (ImGui::RadioButton("None", stage_.sky_type == SkyType::none)) {
        stage_.sky_type = SkyType::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Debug", stage_.sky_type == SkyType::debug)) {
        stage_.sky_type = SkyType::debug;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Skybox", stage_.sky_type == SkyType::skybox)) {
        stage_.sky_type = SkyType::skybox;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Procedural", stage_.sky_type == SkyType::procedural)) {
        stage_.sky_type = SkyType::procedural;
    }

    if (stage_.sky_type == SkyType::procedural) {

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


} // namespace josh::imguihooks
