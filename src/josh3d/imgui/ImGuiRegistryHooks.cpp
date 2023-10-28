#include "ImGuiRegistryHooks.hpp"
#include <imgui.h>


namespace josh {


void ImGuiRegistryHooks::display() {

    auto& hooks = hooks_container_.hook_entries_;

    for (size_t i{ 0 }; i < hooks.size(); ++i) {

        ImGui::PushID(int(i));
        if (ImGui::CollapsingHeader(hooks[i].name.c_str())) {

            hooks[i].hook(registry_);

        }
        ImGui::PopID();

    }

}





} // namespace josh
