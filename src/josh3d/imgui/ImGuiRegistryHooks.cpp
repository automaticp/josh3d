#include "ImGuiRegistryHooks.hpp"
#include <imgui.h>
#include <entt/entt.hpp>


namespace josh {




void ImGuiRegistryHooks::display() {

    if (hidden) { return; }

    auto& hooks = hooks_container_.hook_entries_;

    ImGui::SetNextWindowSize({ 600.f, 600.f }, ImGuiCond_Once);
    ImGui::SetNextWindowPos({ 0.f, 0.f }, ImGuiCond_Once);
    if (ImGui::Begin("Registry")) {

        for (size_t i{ 0 }; i < hooks.size(); ++i) {

            ImGui::PushID(int(i));
            if (ImGui::CollapsingHeader(hooks[i].name.c_str())) {

                hooks[i].hook(registry_);

            }
            ImGui::PopID();

        }
    } ImGui::End();

}





} // namespace josh
