#include "FXAA.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, FXAA) {

    ImGui::Checkbox("Use FXAA", &stage_.use_fxaa);

    ImGui::SliderFloat(
        "Gamma", &stage_.gamma,
        0.f, 10.f, "%.1f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Abs. Threshold", &stage_.absolute_contrast_threshold,
        0.005f, 0.f, 1.f, "%.4f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Rel. Threshold", &stage_.relative_contrast_threshold,
        0.005f, 0.f, 1.f, "%.4f", ImGuiSliderFlags_Logarithmic
    );

}

