#include "AllOverlayHooks.hpp"
#include "detail/SimpleStageHookMacro.hpp"
// IWYU pragma: begin_keep
#include "stages/overlay/CSMDebug.hpp"
#include "stages/overlay/GBufferDebug.hpp"
#include "stages/overlay/SSAODebug.hpp"
#include "stages/overlay/SceneOverlays.hpp"
// IWYU pragma: end_keep
#include "GLAPILimits.hpp"
#include "EnumUtils.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <iterator>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, CSMDebug) {

    const char* mode_names[] = {
        "None",
        "Views",
        "Maps",
    };

    using Mode = stages::overlay::CSMDebug::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 3))
    {
        stage_.mode = Mode{ mode_id };
    }


    if (stage_.mode == Mode::Maps) {

        int max_cascade_id = static_cast<int>(stage_.num_cascades()) - 1;
        int cascade_id = static_cast<int>(stage_.cascade_id);
        if (ImGui::SliderInt("Cascade ID", &cascade_id, 0, max_cascade_id)) {
            stage_.cascade_id = cascade_id;
        }

    }

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, GBufferDebug) {

    const char* mode_names[] = {
        "None",
        "Albedo",
        "Specular",
        "Position",
        "Depth",
        "Depth (Linear)",
        "Normals",
        "Draw Region",
        "Object ID"
    };

    using Mode = stages::overlay::GBufferDebug::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 5))
    {
        stage_.mode = Mode{ mode_id };
    }

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, SSAODebug) {

    const char* mode_names[] = {
        "None",
        "Noisy Occlusion",
        "Occlusion",
    };

    using Mode = stages::overlay::SSAODebug::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 3))
    {
        stage_.mode = Mode{ mode_id };
    }

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, SceneOverlays) {

    ImGui::SeparatorText("Selected Highlight");

    {
        auto& params = stage_.selected_highlight_params;

        ImGui::Checkbox("Show Highlight", &params.show_overlay);
        ImGui::ColorEdit4("Outline", value_ptr(params.outline_color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::ColorEdit4("Fill", value_ptr(params.inner_fill_color), ImGuiColorEditFlags_DisplayHSV);
        auto [min, max] = glapi::limits::aliased_line_width_range();
        ImGui::SliderFloat("Outline Width", &params.outline_width, min, max / 2.f, "%.0f", ImGuiSliderFlags_Logarithmic);
    }

    ImGui::SeparatorText("Bounding Volumes");

    {
        auto& params = stage_.bounding_volumes_params;

        ImGui::Checkbox("Show Bounding Volumes", &params.show_volumes);
        ImGui::Checkbox("Selected Only##BV", &params.selected_only);
        ImGui::ColorEdit3("Color##BV", value_ptr(params.line_color), ImGuiColorEditFlags_DisplayHSV);
        auto [min, max] = glapi::limits::aliased_line_width_range();
        ImGui::SliderFloat("Line Width##BV", &params.line_width, min, max, "%.0f", ImGuiSliderFlags_Logarithmic);
    }

    ImGui::SeparatorText("Relationship Lines");

    {
        auto& params = stage_.scene_graph_lines_params;

        ImGui::Checkbox("Show Lines##RL", &params.show_lines);
        ImGui::BeginDisabled();
        ImGui::Checkbox("Selected Only##RL", &params.selected_only); // TODO
        ImGui::EndDisabled();
        ImGui::Checkbox("Use AABB Midpoints", &params.use_aabb_midpoints);
        ImGui::SliderFloat("Dash Size##RL", &params.dash_size, 0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::ColorEdit4("Color##RL", value_ptr(params.line_color), ImGuiColorEditFlags_DisplayHSV);
        auto [min, max] = glapi::limits::aliased_line_width_range();
        ImGui::SliderFloat("Line Width##RL", &params.line_width, min, max, "%.0f", ImGuiSliderFlags_Logarithmic);
    }

    ImGui::SeparatorText("Skeletons");

    {
        auto& params = stage_.skeleton_params;

        ImGui::Checkbox("Show Skeleton##SK", &params.show_skeleton);
        ImGui::Checkbox("Selected Only##SK", &params.selected_only);
        ImGui::SliderFloat("Joint Scale", &params.joint_scale, 0.f, 1.f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::ColorEdit3("Joint Color", value_ptr(params.joint_color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::SliderFloat("Bone Dash Size##RL", &params.bone_dash_size, 0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        auto [min, max] = glapi::limits::aliased_line_width_range();
        ImGui::SliderFloat("Bone Width##SK", &params.bone_width, min, max, "%.0f", ImGuiSliderFlags_Logarithmic);
        ImGui::ColorEdit4("Bone Color##SK", value_ptr(params.bone_color), ImGuiColorEditFlags_DisplayHSV);
    }

}
