#include "AllPrimaryHooks.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/primary/CascadedShadowMapping.hpp" // IWYU pragma: keep
#include "stages/primary/DeferredGeometry.hpp"      // IWYU pragma: keep
#include "stages/primary/DeferredShading.hpp"       // IWYU pragma: keep
#include "stages/primary/LightDummies.hpp"          // IWYU pragma: keep
#include "stages/primary/PointShadowMapping.hpp"    // IWYU pragma: keep
#include "stages/primary/Sky.hpp"                   // IWYU pragma: keep
#include "stages/primary/SSAO.hpp"                  // IWYU pragma: keep
#include <imgui.h>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, CascadedShadowMapping) {

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


    if (ImGui::TreeNode("Ambient Occlusion")) {

        ImGui::Checkbox("Use Ambient Occlusion", &stage_.use_ambient_occlusion);

        ImGui::SliderFloat(
            "AO Power", &stage_.ambient_occlusion_power,
            0.01f, 100.f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Point Lights/Shadows")) {

        ImGui::SliderFloat(
            "Fade Start", &stage_.plight_fade_start_fraction,
            0.0, 1.0, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Fade Length", &stage_.plight_fade_length_fraction,
            0.0, 1.0, "%.3f", ImGuiSliderFlags_Logarithmic
        );


        ImGui::SliderFloat2(
            "Shadow Bias",
            glm::value_ptr(stage_.point_params.bias_bounds),
            0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderInt(
            "PCF Extent", &stage_.point_params.pcf_extent, 0, 6
        );

        ImGui::SliderFloat(
            "PCF Offset", &stage_.point_params.pcf_offset,
            0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("CSM Shadows")) {

        ImGui::SliderFloat(
            "Base Bias, tx",
            &stage_.dir_params.base_bias_tx,
            0.01f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );


        ImGui::Checkbox("Blend Cascades", &stage_.dir_params.blend_cascades);

        ImGui::SliderFloat(
            "Blend, inner tx",
            &stage_.dir_params.blend_size_inner_tx,
            0.1f, 1000.f, "%.1f", ImGuiSliderFlags_Logarithmic
        );


        ImGui::SliderInt(
            "PCF Extent", &stage_.dir_params.pcf_extent, 0, 12
        );

        ImGui::SliderFloat(
            "PCF Offset, tx", &stage_.dir_params.pcf_offset,
            0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
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

    auto& maps = stage_.view_output().point_shadow_maps_tgt;

    int resolution = maps.resolution().width;
    if (ImGui::SliderInt("Resolution", &resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.resize_maps(Size2I{ resolution, resolution });
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


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, SSAO) {

    ImGui::Checkbox("Enable Occlusion Sampling", &stage_.enable_occlusion_sampling);

    ImGui::SliderFloat(
        "Resolution Divisor", &stage_.resolution_divisor, 0.1f, 10.f,
        "%.3f", ImGuiSliderFlags_Logarithmic
    );


    int kernel_size = static_cast<int>(stage_.get_kernel_size());
    if (ImGui::SliderInt("Kernel Size", &kernel_size,
        1, 256, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.set_kernel_size(static_cast<size_t>(kernel_size));
    }

    if (ImGui::Button("Regenerate Kernel")) {
        stage_.regenerate_kernels();
    }


    float min_angle_deg = glm::degrees(stage_.get_min_sample_angle_from_surface_rad());
    if (ImGui::SliderFloat("Min. Angle, Deg", &min_angle_deg,
        0.f, 89.f, "%.1f"))
    {
        stage_.set_min_sample_angle_from_surface_rad(glm::radians(min_angle_deg));
    }

    ImGui::SliderFloat(
        "Radius", &stage_.radius, 0.001f, 1000.f,
        "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::SliderFloat(
        "Bias", &stage_.bias, 0.0001f, 100.f,
        "%.4f", ImGuiSliderFlags_Logarithmic
    );


    using NoiseMode = stages::primary::SSAO::NoiseMode;

    const char* noise_mode_names[] = {
        "Sampled",
        "Generated"
    };

    int mode_id = to_underlying(stage_.noise_mode);
    if (ImGui::ListBox("Noise Mode", &mode_id,
        noise_mode_names, std::size(noise_mode_names), 2))
    {
        stage_.noise_mode = NoiseMode{ mode_id };
    }

    ImGui::BeginDisabled(stage_.noise_mode != NoiseMode::SampledFromTexture);

    Size2I noise_size = stage_.get_noise_texture_size();
    if (ImGui::SliderInt2("Noise Size", &noise_size.width,
        1, 128))
    {
        stage_.set_noise_texture_size(noise_size);
    }

    if (ImGui::Button("Regenerate Noise Texture")) {
        stage_.regenerate_noise_texture();
    }

    ImGui::EndDisabled();

}
