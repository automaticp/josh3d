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

}


} // namespace josh::imguihooks
