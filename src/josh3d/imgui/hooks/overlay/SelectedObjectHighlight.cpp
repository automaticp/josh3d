#include "SelectedObjectHighlight.hpp"
#include "GLAPILimits.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, SelectedObjectHighlight) {

    ImGui::Checkbox("Show Highlight", &stage_.show_overlay);
    ImGui::ColorEdit4("Outline", glm::value_ptr(stage_.outline_color), ImGuiColorEditFlags_DisplayHSV);
    ImGui::ColorEdit4("Fill", glm::value_ptr(stage_.inner_fill_color), ImGuiColorEditFlags_DisplayHSV);
    auto [min, max] = glapi::limits::aliased_line_width_range();
    ImGui::SliderFloat("Outline Width", &stage_.outline_width, min, max / 2.f, "%.0f", ImGuiSliderFlags_Logarithmic);

}
