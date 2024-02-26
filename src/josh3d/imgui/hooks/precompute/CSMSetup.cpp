#include "CSMSetup.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(precompute, CSMSetup) {

    int num_cascades = static_cast<int>(stage_.num_cascades_to_build);
    // TODO: Where do we get the max num cascades?
    if (ImGui::SliderInt("Num Cascades", &num_cascades, 1, 12)) {
        stage_.num_cascades_to_build = static_cast<size_t>(num_cascades);
    }

    int resolution = stage_.resolution.width;
    if (ImGui::SliderInt("Resolution", &resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.resolution = Size2I{ resolution, resolution };
    }

}
