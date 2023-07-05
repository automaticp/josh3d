#include "GBufferStageHook.hpp"
#include <imgui.h>


namespace learn::imguihooks {


void GBufferStageHook::operator()() {


    if (ImGui::TreeNode("Position/Draw")) {

        ImGui::Image(
            reinterpret_cast<ImTextureID>(gbuffer_->position_target().id()),
            ImVec2{ 300.f, 300.f },
            ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f }
        );

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Normals")) {

        ImGui::Image(
            reinterpret_cast<ImTextureID>(gbuffer_->normals_target().id()),
            ImVec2{ 300.f, 300.f },
            ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f }
        );

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Albedo/Spec")) {

        // Doesn't really work with the default imgui backend setup.
        // Since alpha influences transparency, low specularity is not visible.
        ImGui::Image(
            reinterpret_cast<ImTextureID>(gbuffer_->albedo_spec_target().id()),
            ImVec2{ 300.f, 300.f },
            ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f }
        );

        ImGui::TreePop();
    }


}



} // namespace learn::imguihooks
