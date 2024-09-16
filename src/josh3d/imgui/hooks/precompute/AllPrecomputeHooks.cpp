#include "AllPrecomputeHooks.hpp"
#include "detail/SimpleStageHookMacro.hpp"
// IWYU pragma: begin_keep
#include "stages/precompute/PointLightSetup.hpp"
// IWYU pragma: end_keep
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(precompute, PointLightSetup) {

    using enum target_stage_type::Strategy;

    if (ImGui::RadioButton("Fixed Radius", stage_.strategy == FixedRadius)) {
        stage_.strategy = FixedRadius;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("J0 Threshold", stage_.strategy == RadiosityThreshold)) {
        stage_.strategy = RadiosityThreshold;
    }

    if (stage_.strategy == FixedRadius) {
        ImGui::DragFloat("Bounding Radius, m", &stage_.bounding_radius,
            0.1f, 0.001f, 10000.f, "%.5f", ImGuiSliderFlags_Logarithmic);
    }

    if (stage_.strategy == RadiosityThreshold) {
        ImGui::DragFloat("Min. Radiosity, W/m^2", &stage_.radiosity_threshold,
            0.1f, 0.00001f, 10000.f, "%.5f", ImGuiSliderFlags_Logarithmic);
    }

}
