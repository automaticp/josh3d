#include "ImGuiRegistryHooks.hpp"
#include <imgui.h>
#include <entt/entt.hpp>


namespace learn {




void ImGuiRegistryHooks::display() {

    if (hidden) { return; }

    ImGui::SetNextWindowSize({ 600.f, 600.f }, ImGuiCond_Once);
    ImGui::SetNextWindowPos({ 0.f, 0.f }, ImGuiCond_Once);
    if (ImGui::Begin("Registry")) {

        for (size_t i{ 0 }; i < hooks_.size(); ++i) {

            ImGui::PushID(int(i));
            if (ImGui::CollapsingHeader(hooks_[i].name.c_str())) {

                hooks_[i].hook(registry_);

            }
            ImGui::PopID();

        }
    } ImGui::End();

}





} // namespace learn
