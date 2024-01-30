#include "HDR.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, HDR) {

    ImGui::Checkbox("Use Reinhard", &stage_.use_reinhard);


    ImGui::BeginDisabled(stage_.use_reinhard);

    ImGui::Checkbox("Use Exposure", &stage_.use_exposure);

    ImGui::SliderFloat(
        "Exposure", &stage_.exposure,
        0.01f, 10.f, "%.2f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::EndDisabled();

}
