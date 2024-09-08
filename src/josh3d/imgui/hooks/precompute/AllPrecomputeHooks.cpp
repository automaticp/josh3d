#include "AllPrecomputeHooks.hpp"
#include "detail/SimpleStageHookMacro.hpp"
// IWYU pragma: begin_keep
#include "stages/precompute/PointLightSetup.hpp"
// IWYU pragma: end_keep
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(precompute, PointLightSetup) {

    ImGui::SliderFloat(
        "Att. Threshold", &stage_.threshold_fraction,
        0.f, 1.f, "%.5f", ImGuiSliderFlags_Logarithmic
    );

}
