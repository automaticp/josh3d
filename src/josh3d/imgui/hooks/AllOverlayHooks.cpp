#include "AllHooks.hpp"
#include "ImGuiExtras.hpp"
#include "detail/SimpleStageHookMacro.hpp"
// IWYU pragma: begin_keep
#include "stages/overlay/CSMDebug.hpp"
#include "stages/overlay/GBufferDebug.hpp"
#include "stages/overlay/SSAODebug.hpp"
#include "stages/overlay/SceneOverlays.hpp"
// IWYU pragma: end_keep
#include "GLAPILimits.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(CSMDebug)
{
    using Mode = target_stage_type::OverlayMode;

    ImGui::EnumListBox("Overlay", &stage.mode, 3);

    if (stage.mode == Mode::Maps)
    {
        auto cascade_idx = stage.current_cascade_idx();
        if (ImGui::SliderScalar("Cascade", &cascade_idx, 0, stage.num_cascades_hint() - 1))
            stage.select_cascade(cascade_idx);
    }
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(GBufferDebug)
{
    ImGui::EnumListBox("Overlay", &stage.mode, 5);
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(SSAODebug)
{
    ImGui::EnumListBox("Overlay", &stage.mode, 3);
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(SceneOverlays)
{
    ImGui::SeparatorText("Selected Highlight");
    {
        auto& params = stage.selected_highlight_params;
        ImGui::Checkbox("Show Highlight", &params.show_overlay);
        ImGui::ColorEdit4("Outline", value_ptr(params.outline_color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::ColorEdit4("Fill", value_ptr(params.inner_fill_color), ImGuiColorEditFlags_DisplayHSV);
        auto [min, max] = glapi::get_limit(LimitRF::AliasedLineWidthRange);
        ImGui::SliderFloat("Outline Width", &params.outline_width, min, max / 2.f, "%.0f", ImGuiSliderFlags_Logarithmic);
    }

    ImGui::SeparatorText("Bounding Volumes");
    {
        auto& params = stage.bounding_volumes_params;
        ImGui::Checkbox("Show Bounding Volumes", &params.show_volumes);
        ImGui::Checkbox("Selected Only##BV", &params.selected_only);
        ImGui::ColorEdit3("Color##BV", value_ptr(params.line_color), ImGuiColorEditFlags_DisplayHSV);
        auto [min, max] = glapi::get_limit(LimitRF::AliasedLineWidthRange);
        ImGui::SliderFloat("Line Width##BV", &params.line_width, min, max, "%.0f", ImGuiSliderFlags_Logarithmic);
    }

    ImGui::SeparatorText("Relationship Lines");
    {
        auto& params = stage.scene_graph_lines_params;
        ImGui::Checkbox("Show Lines##RL", &params.show_lines);
        ImGui::BeginDisabled();
        ImGui::Checkbox("Selected Only##RL", &params.selected_only); // TODO
        ImGui::EndDisabled();
        ImGui::Checkbox("Use AABB Midpoints", &params.use_aabb_midpoints);
        ImGui::SliderFloat("Dash Size##RL", &params.dash_size, 0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::ColorEdit4("Color##RL", value_ptr(params.line_color), ImGuiColorEditFlags_DisplayHSV);
        auto [min, max] = glapi::get_limit(LimitRF::AliasedLineWidthRange);
        ImGui::SliderFloat("Line Width##RL", &params.line_width, min, max, "%.0f", ImGuiSliderFlags_Logarithmic);
    }

    ImGui::SeparatorText("Skeletons");
    {
        auto& params = stage.skeleton_params;
        ImGui::Checkbox("Show Skeleton##SK", &params.show_skeleton);
        ImGui::Checkbox("Selected Only##SK", &params.selected_only);
        ImGui::SliderFloat("Joint Scale", &params.joint_scale, 0.f, 1.f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::ColorEdit3("Joint Color", value_ptr(params.joint_color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::SliderFloat("Bone Dash Size##RL", &params.bone_dash_size, 0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        auto [min, max] = glapi::get_limit(LimitRF::AliasedLineWidthRange);
        ImGui::SliderFloat("Bone Width##SK", &params.bone_width, min, max, "%.0f", ImGuiSliderFlags_Logarithmic);
        ImGui::ColorEdit4("Bone Color##SK", value_ptr(params.bone_color), ImGuiColorEditFlags_DisplayHSV);
    }
}
