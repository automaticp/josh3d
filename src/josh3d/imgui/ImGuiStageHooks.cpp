#include "ImGuiStageHooks.hpp"
#include <imgui.h>


namespace josh {


void ImGuiStageHooks::display() {

    auto display_hooks = [](auto& hooks) {
        for (size_t i{ 0 }; i < hooks.size(); ++i) {

            ImGui::PushID(int(i));
            if (ImGui::TreeNode(hooks[i].name.c_str())) {

                hooks[i].hook();

                ImGui::TreePop();
            }
            ImGui::PopID();

        }
    };


    if (ImGui::CollapsingHeader("Precompute")) {
        display_hooks(hooks_container_.precompute_hook_entries_);
    }

    if (ImGui::CollapsingHeader("Primary")) {
        display_hooks(hooks_container_.primary_hook_entries_);
    }

    if (ImGui::CollapsingHeader("Postprocessing")) {
        display_hooks(hooks_container_.pp_hook_entries_);
    }

    if (ImGui::CollapsingHeader("Overlays")) {
        display_hooks(hooks_container_.overlay_hook_entries_);
    }

}




} // namespace josh
