#include "AllPostprocessHooks.hpp"
#include "detail/SimpleStageHookMacro.hpp"
// IWYU pragma: begin_keep
#include "stages/postprocess/Bloom.hpp"
#include "stages/postprocess/Fog.hpp"
#include "stages/postprocess/FXAA.hpp"
#include "stages/postprocess/GammaCorrection.hpp"
#include "stages/postprocess/HDR.hpp"
#include "stages/postprocess/HDREyeAdaptation.hpp"
// IWYU pragma: end_keep
#include "ImGuiHelpers.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <cfloat>




JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, Bloom) {

    ImGui::Checkbox("Use Bloom", &stage_.use_bloom);

    ImGui::SliderFloat2(
        "Threshold", glm::value_ptr(stage_.threshold_bounds),
        0.f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::SliderFloat(
        "Offset Scale", &stage_.offset_scale,
        0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );


    auto num_iterations = static_cast<int>(stage_.blur_iterations);
    if (ImGui::SliderInt("Num Iterations", &num_iterations,
        1, 128, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.blur_iterations = num_iterations;
    }


    if (ImGui::TreeNode("Gaussian Blur")) {

        ImGui::DragFloat(
            "Range [-x, +x]", &stage_.gaussian_sample_range,
            0.1f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        auto num_samples = static_cast<int>(stage_.gaussian_samples);
        if (ImGui::SliderInt(
                "Num Samples", &num_samples,
                0, 15, "%d", ImGuiSliderFlags_Logarithmic
            ))
        {
            stage_.gaussian_samples = num_samples;
        }

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Bloom Texture")) {

        ImGui::Unindent();

        const float w = ImGui::GetContentRegionAvail().x;
        const float h = w / stage_.blur_texture_resolution().aspect_ratio();

        imgui::ImageGL(void_id(stage_.blur_texture().id()), { w, h });

        ImGui::Indent();

        ImGui::TreePop();
    }

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, Fog) {

    using FogType = stages::postprocess::Fog::FogType;

    ImGui::ColorEdit3(
        "Fog Color", glm::value_ptr(stage_.fog_color),
        ImGuiColorEditFlags_DisplayHSV
    );

    if (ImGui::RadioButton("Disabled", stage_.fog_type == FogType::None)) {
        stage_.fog_type = FogType::None;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Uniform", stage_.fog_type == FogType::Uniform)) {
        stage_.fog_type = FogType::Uniform;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Barometric", stage_.fog_type == FogType::Barometric)) {
        stage_.fog_type = FogType::Barometric;
    }


    if (stage_.fog_type == FogType::Uniform) {
        auto& params = stage_.uniform_fog_params;

        ImGui::DragFloat(
            "Mean Free Path", &params.mean_free_path,
            1.f, 0.1f, 1.e4f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::DragFloat(
            "Distance Power", &params.distance_power,
            0.025f, -16.f, 16.f
        );

        ImGui::DragFloat(
            "Z-far Cutoff", &params.cutoff_offset,
            0.1f, 0.01f, 1.e2f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }


    if (stage_.fog_type == FogType::Barometric) {
        auto& params = stage_.barometric_fog_params;

        ImGui::DragFloat(
            "Scale Height", &params.scale_height,
            1.f, 0.1f, 1.e4f, "%.1f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::DragFloat(
            "Base Height", &params.base_height,
            1.f, -FLT_MAX, FLT_MAX, "%.3f"
        );

        ImGui::DragFloat(
            "MFP at Base Height", &params.base_mean_free_path,
            1.f, 0.1f, 1.e4f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

    }


}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, FXAA) {

    ImGui::Checkbox("Use FXAA", &stage_.use_fxaa);

    ImGui::SliderFloat(
        "Gamma", &stage_.gamma,
        0.f, 10.f, "%.1f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Abs. Threshold", &stage_.absolute_contrast_threshold,
        0.005f, 0.f, 1.f, "%.4f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Rel. Threshold", &stage_.relative_contrast_threshold,
        0.005f, 0.f, 1.f, "%.4f", ImGuiSliderFlags_Logarithmic
    );

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, GammaCorrection) {

    ImGui::Checkbox("Use sRGB", &stage_.use_srgb);


    ImGui::BeginDisabled(stage_.use_srgb);

    ImGui::SliderFloat("Gamma", &stage_.gamma, 0.0f, 10.f, "%.1f");

    ImGui::EndDisabled();

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, HDR) {

    ImGui::Checkbox("Use Reinhard", &stage_.use_reinhard);


    ImGui::BeginDisabled(stage_.use_reinhard);

    ImGui::Checkbox("Use Exposure", &stage_.use_exposure);

    ImGui::SliderFloat(
        "Exposure", &stage_.exposure,
        0.01f, 10.f, "%.2f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::EndDisabled();

}


JOSH3D_SIMPLE_STAGE_HOOK_BODY(postprocess, HDREyeAdaptation) {

    ImGui::Checkbox("Use Adaptation", &stage_.use_adaptation);

    if (ImGui::TreeNode("Adjust Screen Value (Slow)")) {

        float val = stage_.get_screen_value();
        if (ImGui::DragFloat("Screen Value", &val,
            0.5f, 0.0f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            stage_.set_screen_value(val);
        }

        ImGui::TreePop();
    }

    ImGui::SliderFloat2(
        "Value Range", glm::value_ptr(stage_.value_range),
        0.f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::SliderFloat(
        "Adaptation Rate", &stage_.adaptation_rate,
        0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Exposure Factor", &stage_.exposure_factor,
        0.5f, 0.0, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    int num_y_samples = static_cast<int>(stage_.num_y_sample_blocks);
    if (ImGui::SliderInt(
        "Num Y Sample Blocks", &num_y_samples,
        1, 1024, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.num_y_sample_blocks = num_y_samples;
    }

    auto dims = stage_.get_sampling_block_dims();
    ImGui::Text(
        "Num Blocks: (%lu, %lu)[%lu]",
        dims.width, dims.height, dims.area()
    );

    ImGui::Text(
        "Block Size: (%lu, %lu)[%lu]",
        stage_.block_dims.width, stage_.block_dims.height, stage_.block_size
    );

    ImGui::Text(
        "Num Samples: (%lu, %lu)[%lu]",
        dims.width * 8, dims.height * 8, dims.area() * 64
    );


}
