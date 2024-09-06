#include "AllPrecomputeHooks.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/precompute/CSMSetup.hpp"        // IWYU pragma: keep
#include "stages/precompute/PointLightSetup.hpp" // IWYU pragma: keep
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

    ImGui::SliderFloat("Log Weight", &stage_.split_log_weight, 0.f, 1.f);
    ImGui::DragFloat("Split Bias", &stage_.split_bias, 1.f, 0.f, FLT_MAX, "%.1f");

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(precompute, PointLightSetup) {

    ImGui::SliderFloat(
        "Att. Threshold", &stage_.threshold_fraction,
        0.f, 1.f, "%.5f", ImGuiSliderFlags_Logarithmic
    );

}
