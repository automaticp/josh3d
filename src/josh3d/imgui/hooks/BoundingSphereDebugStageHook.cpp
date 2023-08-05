#include "BoundingSphereDebugStageHook.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

void josh::imguihooks::BoundingSphereDebugStageHook::operator()() {

    ImGui::Checkbox("Show Bounding Spheres", &stage_.display);
    ImGui::ColorEdit3(
        "Color", glm::value_ptr(stage_.sphere_color),
        ImGuiColorEditFlags_DisplayHSV
    );

}
