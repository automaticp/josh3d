#include "SelectedObjectHighlight.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, SelectedObjectHighlight) {

    ImGui::Checkbox("Show Highlight", &stage_.show_overlay);
    ImGui::ColorEdit4("Outline", glm::value_ptr(stage_.outline_color), ImGuiColorEditFlags_DisplayHSV);
    ImGui::ColorEdit4("Fill", glm::value_ptr(stage_.inner_fill_color), ImGuiColorEditFlags_DisplayHSV);

}
