#include "LightDummies.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, LightDummies) {

    ImGui::Checkbox("Show Light Dummies", &stage_.display);

    ImGui::Checkbox("Attenuate Color", &stage_.attenuate_color);

    ImGui::SliderFloat(
        "Light Dummy Scale", &stage_.light_scale,
        0.001f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}
