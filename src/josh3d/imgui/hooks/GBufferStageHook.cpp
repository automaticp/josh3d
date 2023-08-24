#include "GBufferStageHook.hpp"
#include "ImGuiHelpers.hpp"
#include <imgui.h>


namespace josh::imguihooks {


void GBufferStageHook::operator()() {
    const float aspect = gbuffer_->size().aspect_ratio();

    auto imsize = [&]() -> ImVec2 {
        const float w = ImGui::GetContentRegionAvail().x;
        const float h = w / aspect;
        return { w, h };
    };


    if (ImGui::TreeNode("Position/Draw")) {

        ImGui::Unindent();
        ImGui::ImageGL(void_id(gbuffer_->position_target().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Normals")) {

        ImGui::Unindent();
        ImGui::ImageGL(void_id(gbuffer_->normals_target().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Albedo/Spec")) {

        ImGui::Unindent();
        // Doesn't really work with the default imgui backend setup.
        // Since alpha influences transparency, low specularity is not visible.
        ImGui::ImageGL(void_id(gbuffer_->albedo_spec_target().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }


}



} // namespace josh::imguihooks
