#include "GammaCorrection.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, GammaCorrection) {

    ImGui::Checkbox("Use sRGB", &stage_.use_srgb);


    ImGui::BeginDisabled(stage_.use_srgb);

    ImGui::SliderFloat("Gamma", &stage_.gamma, 0.0f, 10.f, "%.1f");

    ImGui::EndDisabled();

}
