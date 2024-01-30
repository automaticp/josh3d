#include "GBufferStorage.hpp"
#include "ImGuiHelpers.hpp"
#include <imgui.h>


namespace josh::imguihooks::primary {


void GBufferStorage::operator()() {
    const float aspect = gbuffer_->size().aspect_ratio();

    auto imsize = [&]() -> ImVec2 {
        const float w = ImGui::GetContentRegionAvail().x;
        const float h = w / aspect;
        return { w, h };
    };


    if (ImGui::TreeNode("Position/Draw")) {

        ImGui::Unindent();
        imgui::ImageGL(void_id(gbuffer_->position_draw_texture().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Normals")) {

        ImGui::Unindent();
        imgui::ImageGL(void_id(gbuffer_->normals_texture().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Albedo/Spec")) {

        ImGui::Unindent();
        // Doesn't really work with the default imgui backend setup.
        // Since alpha influences transparency, low specularity is not visible.
        imgui::ImageGL(void_id(gbuffer_->albedo_spec_texture().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }


}


} // namespace josh::imguihooks::primary
