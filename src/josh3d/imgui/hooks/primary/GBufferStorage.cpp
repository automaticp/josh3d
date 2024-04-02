#include "GBufferStorage.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "ImGuiHelpers.hpp"
#include <imgui.h>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, GBufferStorage) {

    // TODO: This is largely useless now that we have a GBufferDebug overlay.

    const GBuffer& gbuffer = stage_.view_gbuffer();
    const float aspect = gbuffer.resolution().aspect_ratio();

    auto imsize = [&]() -> ImVec2 {
        const float w = ImGui::GetContentRegionAvail().x;
        const float h = w / aspect;
        return { w, h };
    };

    if (ImGui::TreeNode("Position/Draw")) {

        ImGui::Unindent();
        imgui::ImageGL(void_id(gbuffer.position_draw_texture().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Normals")) {

        ImGui::Unindent();
        imgui::ImageGL(void_id(gbuffer.normals_texture().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Albedo/Spec")) {

        ImGui::Unindent();
        // Doesn't really work with the default imgui backend setup.
        // Since alpha influences transparency, low specularity is not visible.
        imgui::ImageGL(void_id(gbuffer.albedo_spec_texture().id()), imsize());
        ImGui::Indent();

        ImGui::TreePop();
    }


}
