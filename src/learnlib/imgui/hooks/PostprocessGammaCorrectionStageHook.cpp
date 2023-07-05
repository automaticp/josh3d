#include "PostprocessGammaCorrectionStageHook.hpp"
#include <imgui.h>


void learn::imguihooks::PostprocessGammaCorrectionStageHook::operator()() {

    ImGui::Checkbox("Use sRGB", &stage_.use_srgb);


    ImGui::BeginDisabled(stage_.use_srgb);

    ImGui::SliderFloat("Gamma", &stage_.gamma, 0.0f, 10.f, "%.1f");

    ImGui::EndDisabled();

}
