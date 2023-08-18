#include "CascadedShadowMappingStageHook.hpp"
#include <imgui.h>



namespace josh::imguihooks {


void CascadedShadowMappingStageHook::operator()() {

    const size_t min_cascades{ 1 };
    const size_t max_cascades{ stage_.max_cascades() };
    ImGui::SliderScalar(
        "Num Cascades", ImGuiDataType_U64, &builder_.num_cascades_to_build,
        &min_cascades, &max_cascades
    );

    ImGui::SliderInt(
        "New Resolution", &resolution_,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic
    );

    if (ImGui::Button("Change Resolution")) {
        stage_.resize_maps(resolution_, resolution_);
    }
    ImGui::SameLine();
    ImGui::Text(
        "%d -> %d",
        stage_output_->dir_shadow_maps.width(), resolution_
    );

}


} // namespace josh::imguihooks
