#include "AllHooks.hpp"
#include "ImGuiExtras.hpp"
#include "detail/SimpleStageHookMacro.hpp"
// IWYU pragma: begin_keep
#include "stages/postprocess/GaussianBloom.hpp"
#include "stages/postprocess/BloomAW.hpp"
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


JOSH3D_SIMPLE_STAGE_HOOK_BODY(GaussianBloom)
{
    ImGui::Checkbox("Use Bloom", &stage.use_bloom);
    ImGui::SliderFloat2("Threshold", value_ptr(stage.threshold_bounds),
        0.f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Offset Scale", &stage.offset_scale,
        0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderScalar("Num Iterations", &stage.blur_iterations,
        1, 128, {}, ImGuiSliderFlags_Logarithmic);

    if (ImGui::TreeNode("Gaussian Kernel"))
    {
        auto range     = stage.kernel_range();
        auto limb_size = stage.kernel_limb_size();
        if (ImGui::DragFloat("Range [-x, +x]", &range, 0.1f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic) +
            ImGui::SliderScalar("Limb Size", &limb_size, 0, 15, {}, ImGuiSliderFlags_Logarithmic))
        {
            stage.resize_kernel(limb_size, range);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Bloom Texture"))
    {
        ImGui::Unindent();
        const float w = ImGui::GetContentRegionAvail().x;
        const float h = w / stage.target.resolution().aspect_ratio();
        ImGui::ImageGL(stage.target.front_texture().id(), { w, h });
        ImGui::Indent();
        ImGui::TreePop();
    }
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(BloomAW)
{
    ImGui::Checkbox("Use Bloom", &stage.enable_bloom);
    ImGui::SliderScalar("Max Levels", &stage.max_downsample_levels, 1, stage.num_available_levels());
    ImGui::SliderFloat("Bloom Weight", &stage.bloom_weight, 0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Filter Scale, px", &stage.filter_scale_px, 0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(Fog)
{
    using FogType = target_stage_type::FogType;

    ImGui::ColorEdit3("Fog Color", value_ptr(stage.fog_color), ImGuiColorEditFlags_DisplayHSV);
    ImGui::EnumCombo("Type", &stage.fog_type);

    if (stage.fog_type == FogType::Uniform)
    {
        auto& params = stage.uniform_fog_params;

        ImGui::DragFloat("Mean Free Path", &params.mean_free_path,
            1.f, 0.1f, 1.e4f, "%.2f", ImGuiSliderFlags_Logarithmic);

        ImGui::DragFloat("Distance Power", &params.distance_power,
            0.025f, -16.f, 16.f);

        ImGui::DragFloat("Z-far Cutoff", &params.cutoff_offset,
            0.1f, 0.01f, 1.e2f, "%.2f", ImGuiSliderFlags_Logarithmic);
    }

    if (stage.fog_type == FogType::Barometric)
    {
        auto& params = stage.barometric_fog_params;

        ImGui::DragFloat("Scale Height", &params.scale_height,
            1.f, 0.1f, 1.e4f, "%.1f", ImGuiSliderFlags_Logarithmic);

        ImGui::DragFloat("Base Height", &params.base_height,
            1.f, -FLT_MAX, FLT_MAX, "%.3f");

        ImGui::DragFloat("MFP at Base Height", &params.base_mean_free_path,
            1.f, 0.1f, 1.e4f, "%.2f", ImGuiSliderFlags_Logarithmic);
    }
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(FXAA)
{
    ImGui::Checkbox("Use FXAA", &stage.use_fxaa);

    ImGui::SliderFloat("Gamma", &stage.gamma,
        0.f, 10.f, "%.1f", ImGuiSliderFlags_Logarithmic);

    ImGui::DragFloat("Abs. Threshold", &stage.absolute_contrast_threshold,
        0.005f, 0.f, 1.f, "%.4f", ImGuiSliderFlags_Logarithmic);

    ImGui::DragFloat("Rel. Threshold", &stage.relative_contrast_threshold,
        0.005f, 0.f, 1.f, "%.4f", ImGuiSliderFlags_Logarithmic);
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(GammaCorrection)
{
    ImGui::Checkbox("Use sRGB", &stage.use_srgb);
    ImGui::BeginDisabled(stage.use_srgb);
    ImGui::SliderFloat("Gamma", &stage.gamma, 0.0f, 10.f, "%.1f");
    ImGui::EndDisabled();
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(HDR)
{
    ImGui::Checkbox("Use Reinhard", &stage.use_reinhard);
    ImGui::BeginDisabled(stage.use_reinhard);
    ImGui::Checkbox("Use Exposure", &stage.use_exposure);
    ImGui::SliderFloat("Exposure", &stage.exposure,
        0.01f, 10.f, "%.2f", ImGuiSliderFlags_Logarithmic);
    ImGui::EndDisabled();
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(HDREyeAdaptation)
{
    ImGui::Checkbox("Use Adaptation", &stage.use_adaptation);

    if (ImGui::TreeNode("Adjust Screen Value (SLOW)"))
    {
        float val = stage.get_screen_value();
        if (ImGui::DragFloat("Screen Value", &val,
            0.5f, 0.0f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            stage.set_screen_value(val);
        }
        ImGui::TreePop();
    }

    ImGui::SliderFloat2("Value Range", value_ptr(stage.value_range),
        0.f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);

    ImGui::SliderFloat("Adaptation Rate", &stage.adaptation_rate,
        0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);

    ImGui::DragFloat("Exposure Factor", &stage.exposure_factor,
        0.5f, 0.0, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);

    ImGui::SliderScalar("Num Y Sample Blocks", &stage.num_y_sample_blocks,
        1, 1024, "%d", ImGuiSliderFlags_Logarithmic);

    ImGui::Checkbox("Read Back Exposure", &stage.read_back_exposure);

    if (ImGui::TreeNode("Stats"))
    {
        ImGui::Text("Latest Exposure: %.3f",     stage.exposure.exposure);
        ImGui::Text("Latest Screen Value: %.3f", stage.exposure.screen_value);
        ImGui::Text("Latency (Frames): %lu",     stage.exposure.latency_in_frames);
        const auto dims = stage.get_sampling_block_dims();
        ImGui::Text("Num Blocks: (%lu, %lu)[%lu]", dims.width, dims.height, dims.area());
        ImGui::Text("Block Size: (%lu, %lu)[%lu]", stage.block_dims.width, stage.block_dims.height, stage.block_size);
        ImGui::Text("Num Samples: (%lu, %lu)[%lu]", dims.width * 8, dims.height * 8, dims.area() * 64);
        ImGui::TreePop();
    }
}
