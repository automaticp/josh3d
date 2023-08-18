#include "PointShadowMappingStageHook.hpp"
#include <imgui.h>

namespace josh::imguihooks {


void PointShadowMappingStageHook::operator()() {

    ImGui::SliderInt(
        "New Resolution", &resolution_,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic
    );

    const bool change_res = ImGui::Button("Change Resolution");
    ImGui::SameLine();
    ImGui::Text(
        "%d -> %d",
        stage_output_->point_shadow_maps.width(), resolution_
    );

    if (change_res) {
        stage_.resize_maps(resolution_, resolution_);
    }

}


} // namespace josh::imguihooks
