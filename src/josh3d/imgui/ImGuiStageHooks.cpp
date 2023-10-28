#include "ImGuiStageHooks.hpp"
#include <imgui.h>


namespace josh {


void ImGuiStageHooks::display() {

    if (ImGui::CollapsingHeader("Primary")) {
        auto& hooks = hooks_container_.primary_hook_entries_;
        for (size_t i{ 0 }; i < hooks.size(); ++i) {

            ImGui::PushID(int(i));
            if (ImGui::TreeNode(hooks[i].name.c_str())) {

                hooks[i].hook();

                ImGui::TreePop();
            }
            ImGui::PopID();

        }
    }

    if (ImGui::CollapsingHeader("Postprocessing")) {
        auto& hooks = hooks_container_.pp_hook_entries_;
        for (size_t i{ 0 }; i < hooks.size(); ++i) {

            ImGui::PushID(int(i));
            if (ImGui::TreeNode(hooks[i].name.c_str())) {

                hooks[i].hook();

                ImGui::TreePop();
            }
            ImGui::PopID();

        }
    }

    if (ImGui::CollapsingHeader("Overlays")) {
        auto& hooks = hooks_container_.overlay_hook_entries_;
        for (size_t i{ 0 }; i < hooks.size(); ++i) {

            ImGui::PushID(int(i));
            if (ImGui::TreeNode(hooks[i].name.c_str())) {

                hooks[i].hook();

                ImGui::TreePop();
            }
            ImGui::PopID();

        }
    }

}




} // namespace josh
