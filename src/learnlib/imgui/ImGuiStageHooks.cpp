#include "ImGuiStageHooks.hpp"
#include <imgui.h>


namespace learn {




void ImGuiStageHooks::display() {

    if (hidden) { return; }

    ImGui::SetNextWindowSize({ 600.f, 400.f }, ImGuiCond_Once);
    ImGui::SetNextWindowPos({ 0.f, 600.f }, ImGuiCond_Once);
    if (ImGui::Begin("Render Stages")) {

        if (ImGui::CollapsingHeader("Primary")) {
            for (size_t i{ 0 }; i < hooks_.size(); ++i) {

                ImGui::PushID(int(i));
                if (ImGui::TreeNode(hooks_[i].name.c_str())) {

                    hooks_[i].hook();

                    ImGui::TreePop();
                }
                ImGui::PopID();

            }
        }

        if (ImGui::CollapsingHeader("Postprocessing")) {
            for (size_t i{ 0 }; i < pp_hooks_.size(); ++i) {

                ImGui::PushID(int(i));
                if (ImGui::TreeNode(pp_hooks_[i].name.c_str())) {

                    pp_hooks_[i].hook();

                    ImGui::TreePop();
                }
                ImGui::PopID();

            }
        }

    } ImGui::End();
}




} // namespace learn
