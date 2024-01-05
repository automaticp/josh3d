#include "OverlaySelectedStageHook.hpp"
#include <imgui.h>


namespace josh::imguihooks {


void OverlaySelectedStageHook::operator()() {

    ImGui::Checkbox("Show Highlight", &stage_.show_overlay);
    ImGui::ColorEdit4("Outline", glm::value_ptr(stage_.outline_color), ImGuiColorEditFlags_DisplayHSV);
    ImGui::ColorEdit4("Fill", glm::value_ptr(stage_.inner_fill_color), ImGuiColorEditFlags_DisplayHSV);

}


} // namespace josh::imguihooks {
