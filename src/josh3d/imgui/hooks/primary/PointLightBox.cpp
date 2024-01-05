#include "PointLightBox.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, PointLightBox) {

    ImGui::Checkbox("Show Light Boxes", &stage_.display);

    ImGui::SliderFloat(
        "Light Box Scale", &stage_.light_box_scale,
        0.001f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}
