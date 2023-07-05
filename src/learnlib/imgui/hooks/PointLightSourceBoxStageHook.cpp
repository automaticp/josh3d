#include "PointLightSourceBoxStageHook.hpp"
#include <imgui.h>



void learn::imguihooks::PointLightSourceBoxStageHook::operator()() {

    ImGui::SliderFloat(
        "Light Box Scale", &stage_.light_box_scale,
        0.001f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}
