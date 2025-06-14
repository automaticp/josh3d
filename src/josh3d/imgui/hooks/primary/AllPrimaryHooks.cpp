#include "AllPrimaryHooks.hpp"
#include "ImGuiExtras.hpp"
#include "Region.hpp"
#include "detail/SimpleStageHookMacro.hpp"
// IWYU pragma: begin_keep
#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/DeferredGeometry.hpp"
#include "stages/primary/DeferredShading.hpp"
#include "stages/primary/LightDummies.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/Sky.hpp"
#include "stages/primary/SSAO.hpp"
// IWYU pragma: end_keep
#include <imgui.h>
#include <imgui_internal.h>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, CascadedShadowMapping) {

    using Strategy = target_stage_type::Strategy;

    if (ImGui::RadioButton("Singlepass GS", stage_.strategy == Strategy::SinglepassGS)) {
        stage_.strategy = Strategy::SinglepassGS;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Cull Per Cascade", stage_.strategy == Strategy::PerCascadeCulling)) {
        stage_.strategy = Strategy::PerCascadeCulling;
    }

    ImGui::BeginDisabled(stage_.strategy != Strategy::PerCascadeCulling);
    ImGui::Checkbox("MultiDraw Opaque", &stage_.multidraw_opaque);
    ImGui::EndDisabled();


    int num_cascades = int(stage_.num_cascades());
    const int max_cascades = int(stage_.max_cascades());
    if (ImGui::SliderInt("Num Cascades", &num_cascades, 1, max_cascades)) {
        stage_.set_num_cascades(size_t(num_cascades));
    }

    int side_resolution = stage_.side_resolution;
    if (ImGui::SliderInt("Resolution", &side_resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.side_resolution = side_resolution;
    }


    ImGui::SeparatorText("Splits");


    float split_linear_weight = 1.f - stage_.split_log_weight;
    if (ImGui::SliderFloat("Linear Weight", &split_linear_weight,
        0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic))
    {
        stage_.split_log_weight = 1.f - split_linear_weight;
    }
    ImGui::DragFloat("Split Bias", &stage_.split_bias, 1.f, 0.f, FLT_MAX, "%.1f");


    ImGui::SeparatorText("Cascade Blending");


    ImGui::Checkbox("Blend Cascades", &stage_.support_cascade_blending);
    ImGui::BeginDisabled(!stage_.support_cascade_blending);
    ImGui::SliderFloat("Blend, inner tx", &stage_.blend_size_inner_tx,
        0.1f, 1000.f, "%.1f", ImGuiSliderFlags_Logarithmic);
    ImGui::EndDisabled();


    ImGui::SeparatorText("Face Culling");


    ImGui::Checkbox("Face Culling", &stage_.enable_face_culling);
    ImGui::BeginDisabled(!stage_.enable_face_culling);
    const char* face_names[] = {
        "Back",
        "Front",
    };
    int face = stage_.faces_to_cull == Face::Back ? 0 : 1;
    if (ImGui::ListBox("Faces to Cull", &face, face_names, 2, 2)) {
        stage_.faces_to_cull = face == 0 ? Face::Back : Face::Front;
    }
    ImGui::EndDisabled();


    ImGui::Separator();


    ImGui::BeginDisabled(!stage_.view_output().draw_lists_active);
    if (ImGui::TreeNode("Draw Call Stats")) {
        const auto& views      = stage_.view_output().views;
        const auto& drawstates = stage_.view_output().drawstates;

        const ImGuiTableFlags flags =
            ImGuiTableFlags_Borders        |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_NoHostExtendX;
        ImGui::BeginTable("Draw Call Table", 3, flags);
        ImGui::TableSetupColumn("Cascade ID");
        ImGui::TableSetupColumn("Solid");
        ImGui::TableSetupColumn("Alpha-Tested");
        ImGui::TableHeadersRow();
        size_t total_draws_noat{ 0 };
        size_t total_draws_at  { 0 };
        for (const auto i : std::views::iota(size_t{}, views.size())) {
            const size_t draws_at     = drawstates[i].draw_list_at    .size();
            const size_t draws_opaque = drawstates[i].draw_list_opaque.size();
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", int(i));
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", int(draws_opaque));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", int(draws_at));
            total_draws_at   += draws_at;
            total_draws_noat += draws_opaque;
        }
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Total");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", int(total_draws_noat));
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%d", int(total_draws_at));

        ImGui::EndTable();
        ImGui::TreePop();
    }
    ImGui::EndDisabled();

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, DeferredGeometry) {

    ImGui::Checkbox("Backface Culling", &stage_.enable_backface_culling);

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, DeferredShading) {

    using Mode = stages::primary::DeferredShading::Mode;

    if (ImGui::RadioButton("Singlepass", stage_.mode == Mode::SinglePass)) {
        stage_.mode = Mode::SinglePass;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Multipass", stage_.mode == Mode::MultiPass)) {
        stage_.mode = Mode::MultiPass;
    }


    ImGui::SeparatorText("Ambient Occlusion");
    {

        ImGui::Checkbox("Use Ambient Occlusion", &stage_.use_ambient_occlusion);

        ImGui::SliderFloat(
            "AO Power", &stage_.ambient_occlusion_power,
            0.01f, 100.f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }


    ImGui::SeparatorText("Point Lights/Shadows");
    {

        ImGui::SliderFloat(
            "Fade Start", &stage_.plight_fade_start_fraction,
            0.0, 1.0, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat2(
            "Shadow Bias##PSM",
            glm::value_ptr(stage_.point_params.bias_bounds),
            0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderInt(
            "PCF Extent##PSM", &stage_.point_params.pcf_extent, 0, 6
        );

        ImGui::SliderFloat(
            "PCF Offset##PSM", &stage_.point_params.pcf_offset,
            0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

    }



    ImGui::SeparatorText("CSM Shadows");
    {

        ImGui::SliderFloat(
            "Base Bias, tx##CSM",
            &stage_.dir_params.base_bias_tx,
            0.01f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderInt(
            "PCF Extent##CSM", &stage_.dir_params.pcf_extent, 0, 12
        );

        ImGui::SliderFloat(
            "PCF Offset, tx##CSM", &stage_.dir_params.pcf_offset,
            0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }


}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, LightDummies) {

    ImGui::Checkbox("Show Light Dummies", &stage_.display);

    ImGui::Checkbox("Attenuate Color", &stage_.attenuate_color);

    ImGui::SliderFloat(
        "Light Dummy Scale", &stage_.light_scale,
        0.001f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, PointShadowMapping) {

    int side_resolution = stage_.side_resolution;
    if (ImGui::SliderInt("Resolution", &side_resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.side_resolution = side_resolution;
    }

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, Sky) {

    using SkyType = stages::primary::Sky::SkyType;

    if (ImGui::RadioButton("None", stage_.sky_type == SkyType::None)) {
        stage_.sky_type = SkyType::None;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Debug", stage_.sky_type == SkyType::Debug)) {
        stage_.sky_type = SkyType::Debug;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Skybox", stage_.sky_type == SkyType::Skybox)) {
        stage_.sky_type = SkyType::Skybox;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Procedural", stage_.sky_type == SkyType::Procedural)) {
        stage_.sky_type = SkyType::Procedural;
    }

    if (stage_.sky_type == SkyType::Procedural) {

        auto& params = stage_.procedural_sky_params;

        ImGui::ColorEdit3(
            "Sky Color", glm::value_ptr(params.sky_color),
            ImGuiColorEditFlags_DisplayHSV
        );

        ImGui::ColorEdit3(
            "Sun Color", glm::value_ptr(params.sun_color),
            ImGuiColorEditFlags_DisplayHSV
        );

        ImGui::SliderFloat(
            "Sun Diameter, deg", &params.sun_size_deg,
            0.f, 45.f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, SSAO)
{
    ImGui::Checkbox("Enable Sampling", &stage_.enable_sampling);

    ImGui::SliderFloat(
        "Resolution Divisor", &stage_.resolution_divisor, 0.1f, 10.f,
        "%.3f", ImGuiSliderFlags_Logarithmic
    );

    int kernel_size = int(stage_.kernel_size());
    if (ImGui::SliderInt("Kernel Size", &kernel_size,
        1, 256, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.regenerate_kernel(usize(kernel_size));
    }

    if (ImGui::Button("Regenerate Kernel"))
        stage_.regenerate_kernel(stage_.kernel_size());

    float min_angle_deg = glm::degrees(stage_.deflection_rad());
    if (ImGui::SliderFloat("Min. Angle, Deg", &min_angle_deg,
        0.f, 89.f, "%.1f"))
    {
        stage_.set_deflection_rad(glm::radians(min_angle_deg));
    }

    ImGui::SliderFloat(
        "Radius", &stage_.radius, 0.001f, 1000.f,
        "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::SliderFloat(
        "Bias", &stage_.bias, 0.0001f, 100.f,
        "%.4f", ImGuiSliderFlags_Logarithmic
    );

    using NoiseMode = stages::primary::SSAONoiseMode;

    ImGui::EnumListBox("Noise Mode", &stage_.noise_mode, 2);

    ImGui::BeginDisabled(stage_.noise_mode != NoiseMode::SampledFromTexture);

    Extent2I noise_resolution = stage_.noise_texture_resolution();
    if (ImGui::SliderInt2("Noise Size", &noise_resolution.width, 1, 128))
        stage_.regenerate_noise_texture(noise_resolution);

    if (ImGui::Button("Regenerate Noise Texture"))
        stage_.regenerate_noise_texture(stage_.noise_texture_resolution());

    ImGui::EndDisabled();

}
