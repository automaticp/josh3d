#include "PointLightSourceBoxStageHook.hpp"
#include <imgui.h>



void josh::imguihooks::PointLightSourceBoxStageHook::operator()() {

    ImGui::Checkbox("Show Light Boxes", &stage_.display);

    ImGui::SliderFloat(
        "Light Box Scale", &stage_.light_box_scale,
        0.001f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}
