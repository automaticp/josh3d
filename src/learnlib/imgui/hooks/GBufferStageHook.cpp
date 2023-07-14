#include "GBufferStageHook.hpp"
#include "ImGuiHelpers.hpp"
#include <imgui.h>


namespace josh::imguihooks {


void GBufferStageHook::operator()() {


    if (ImGui::TreeNode("Position/Draw")) {

        ImGui::ImageGL(void_id(gbuffer_->position_target().id()), { 300.f, 300.f });

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Normals")) {

        ImGui::ImageGL(void_id(gbuffer_->normals_target().id()), { 300.f, 300.f });

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Albedo/Spec")) {

        // Doesn't really work with the default imgui backend setup.
        // Since alpha influences transparency, low specularity is not visible.
        ImGui::ImageGL(void_id(gbuffer_->albedo_spec_target().id()), { 300.f, 300.f });

        ImGui::TreePop();
    }


}



} // namespace josh::imguihooks
