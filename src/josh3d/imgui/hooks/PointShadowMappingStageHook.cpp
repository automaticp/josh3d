#include "PointShadowMappingStageHook.hpp"
#include <glm/gtc/type_ptr.hpp>
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
        stage_output_->point_shadow_maps.size().width, resolution_
    );

    if (change_res) {
        stage_.resize_maps({ resolution_, resolution_ });
    }

    ImGui::DragFloat2(
        "Z Near/Far", glm::value_ptr(stage_.z_near_far()),
        1.0f, 0.001f, 1e4f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}


} // namespace josh::imguihooks
