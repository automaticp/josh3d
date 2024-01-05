#include "BoundingSphereDebug.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, BoundingSphereDebug) {

    ImGui::Checkbox("Show Bounding Spheres", &stage_.display);
    ImGui::ColorEdit3(
        "Color", glm::value_ptr(stage_.sphere_color),
        ImGuiColorEditFlags_DisplayHSV
    );

}
