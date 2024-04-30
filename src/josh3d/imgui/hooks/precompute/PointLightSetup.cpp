#include "PointLightSetup.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(precompute, PointLightSetup) {

    ImGui::SliderFloat(
        "Att. Threshold", &stage_.threshold_fraction,
        0.f, 1.f, "%.5f", ImGuiSliderFlags_Logarithmic
    );

}
