#include "BoundingSphereDebug.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, BoundingSphereDebug) {

    ImGui::Checkbox("Show Bounding Spheres", &stage_.display);
    ImGui::Checkbox("Selected Only", &stage_.selected_only);
    ImGui::ColorEdit3(
        "Color", glm::value_ptr(stage_.line_color),
        ImGuiColorEditFlags_DisplayHSV
    );
    ImGui::SliderFloat("Line Width", &stage_.line_width, 1.f, 10.f, "%.0f");

}
