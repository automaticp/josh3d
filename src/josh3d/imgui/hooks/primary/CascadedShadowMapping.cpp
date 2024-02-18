#include "CascadedShadowMapping.hpp"
#include <imgui.h>


namespace josh::imguihooks::primary {


void CascadedShadowMapping::operator()() {

    auto& maps = stage_.view_output().dir_shadow_maps_tgt.depth_attachment();

    ImGui::SliderInt(
        "New Resolution", &resolution_,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic
    );

    if (ImGui::Button("Change Resolution")) {
        stage_.resize_maps({ resolution_, resolution_ });
    }
    ImGui::SameLine();
    ImGui::Text(
        "%d -> %d",
        maps.size().width, resolution_
    );

}


} // namespace josh::imguihooks::primary
