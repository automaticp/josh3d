#include "AllHooks.hpp"
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
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <imgui_internal.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(CascadedShadowMapping)
{
    ImGui::EnumListBox("Strategy", &stage.strategy);

    auto num_cascades    = stage.num_cascades();
    auto side_resolution = stage.side_resolution();
    if (ImGui::SliderScalar("Num Cascades", &num_cascades, 1, stage.max_cascades()) +
        ImGui::SliderScalar("Side Resolution", &side_resolution, 128, 8192, {}, ImGuiSliderFlags_Logarithmic))
    {
        stage.resize_maps(side_resolution, num_cascades);
    }

    ImGui::SeparatorText("Splits");

    float split_linear_weight = 1.f - stage.split_log_weight;
    if (ImGui::SliderFloat("Linear Weight", &split_linear_weight,
            0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic))
    {
        stage.split_log_weight = 1.f - split_linear_weight;
    }
    ImGui::DragFloat("Split Bias", &stage.split_bias, 1.f, 0.f, FLT_MAX, "%.1f");

    ImGui::SeparatorText("Cascade Blending");

    ImGui::Checkbox("Blend Cascades", &stage.support_cascade_blending);
    ImGui::BeginDisabled(not stage.support_cascade_blending);
    ImGui::SliderFloat("Blend, inner tx", &stage.blend_size_inner_tx,
        0.1f, 1000.f, "%.1f", ImGuiSliderFlags_Logarithmic);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Face Culling");

    ImGui::Checkbox("Face Culling", &stage.enable_face_culling);
    ImGui::BeginDisabled(not stage.enable_face_culling);
    ImGui::EnumListBox("Faces to Cull", &stage.faces_to_cull, 0);
    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::BeginDisabled(not stage.cascades.draw_lists_active);
    if (ImGui::TreeNode("Draw Call Stats"))
    {
        const auto& views      = stage.cascades.views;
        const auto& drawstates = stage.cascades.drawstates;

        const ImGuiTableFlags flags =
            ImGuiTableFlags_Borders        |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_NoHostExtendX;
        ImGui::BeginTable("Draw Call Table", 3, flags);
        ImGui::TableSetupColumn("Cascade ID");
        ImGui::TableSetupColumn("Solid");
        ImGui::TableSetupColumn("Alpha-Tested");
        ImGui::TableHeadersRow();
        usize total_draws_opaque  = 0;
        usize total_draws_atested = 0;
        for (const uindex i : irange(views.size()))
        {
            const usize draws_atested = drawstates[i].drawlist_atested.size();
            const usize draws_opaque  = drawstates[i].drawlist_opaque.size();
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", draws_opaque);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", draws_atested);
            total_draws_atested += draws_atested;
            total_draws_opaque  += draws_opaque;
        }
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Total");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zu", total_draws_opaque);
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%zu", total_draws_atested);

        ImGui::EndTable();
        ImGui::TreePop();
    }
    ImGui::EndDisabled();
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(DeferredGeometry)
{
    ImGui::Checkbox("Backface Culling", &stage.backface_culling);
    ImGui::EnumListBox("Strategy", &stage.strategy);
    if (stage.strategy == josh::DeferredGeometry::Strategy::BatchedMDI)
        ImGui::Text("Max Batch Size: %u", stage.max_batch_size());
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(DeferredShading)
{
    ImGui::EnumListBox("Mode", &stage.mode, 0);

    ImGui::SeparatorText("Ambient Occlusion");

    ImGui::Checkbox("Use Ambient Occlusion", &stage.use_ambient_occlusion);
    ImGui::SliderFloat("AO Power", &stage.ambient_occlusion_power,
        0.01f, 100.f, "%.2f", ImGuiSliderFlags_Logarithmic);

    ImGui::SeparatorText("Point Lights/Shadows");

    ImGui::SliderFloat("Fade Start", &stage.plight_fade_start_fraction,
        0.0, 1.0, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat2("Shadow Bias##PSM", value_ptr(stage.point_params.bias_bounds),
        0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderInt("PCF Extent##PSM", &stage.point_params.pcf_extent, 0, 6);
    ImGui::SliderFloat("PCF Offset##PSM", &stage.point_params.pcf_offset,
        0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);

    ImGui::SeparatorText("CSM Shadows");

    ImGui::SliderFloat("Base Bias, tx##CSM", &stage.dir_params.base_bias_tx,
        0.01f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderInt("PCF Extent##CSM", &stage.dir_params.pcf_extent, 0, 12);
    ImGui::SliderFloat("PCF Offset, tx##CSM", &stage.dir_params.pcf_offset,
        0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(LightDummies)
{
    ImGui::Checkbox("Show Light Dummies", &stage.display);
    ImGui::Checkbox("Attenuate Color", &stage.attenuate_color);
    ImGui::SliderFloat("Light Dummy Scale", &stage.light_scale,
        0.001f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic);
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(PointShadowMapping)
{
    auto side_resolution = stage.side_resolution();
    if (ImGui::SliderScalar("Resolution", &side_resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage.resize_maps(side_resolution);
    }
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(Sky)
{
    using SkyType = target_stage_type::SkyType;

    ImGui::EnumListBox("Type", &stage.sky_type, 0);

    if (stage.sky_type == SkyType::Procedural)
    {
        auto& params = stage.procedural_sky_params;

        ImGui::ColorEdit3("Sky Color", value_ptr(params.sky_color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::ColorEdit3("Sun Color", value_ptr(params.sun_color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::SliderFloat("Sun Diameter, deg", &params.sun_size_deg,
            0.f, 45.f, "%.2f", ImGuiSliderFlags_Logarithmic);
    }
}

JOSH3D_SIMPLE_STAGE_HOOK_BODY(SSAO)
{
    ImGui::Checkbox("Enable Sampling", &stage.enable_sampling);
    ImGui::SliderFloat("Resolution Divisor", &stage.resolution_divisor,
        0.1f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic);

    auto kernel_size   = stage.kernel_size();
    auto min_angle_deg = glm::degrees(stage.deflection_rad());
    if (ImGui::SliderScalar("Kernel Size", &kernel_size, 1, 256, {}, ImGuiSliderFlags_Logarithmic) +
        ImGui::SliderFloat("Min. Angle, Deg", &min_angle_deg, 0.f, 89.f, "%.1f") +
        ImGui::Button("Regenerate Kernel"))
    {
        stage.regenerate_kernel(kernel_size, glm::radians(min_angle_deg));
    }

    ImGui::SliderFloat("Radius", &stage.radius,
        0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Bias", &stage.bias,
        0.0001f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic);

    using BlurMode = target_stage_type::BlurMode;
    ImGui::EnumListBox("Blur Mode", &stage.blur_mode);

    if (stage.blur_mode == BlurMode::Bilateral)
    {
        ImGui::SliderFloat("Blur Depth Limit", &stage.depth_limit,
            0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        auto limb_size = stage.blur_kernel_limb_size();
        if (ImGui::SliderScalar("Blur Kernel Limb Size", &limb_size, 0, 16))
            stage.resize_blur_kernel(limb_size);
        ImGui::SliderScalar("Num Blur Passes", &stage.num_blur_passes, 0, 8);
    }

    using NoiseMode = target_stage_type::NoiseMode;
    ImGui::EnumListBox("Noise Mode", &stage.noise_mode, 0);

    if (stage.noise_mode == NoiseMode::SampledFromTexture)
    {
        Extent2I noise_resolution = stage.noise_texture_resolution();
        if (ImGui::SliderInt2("Noise Size", &noise_resolution.width, 1, 128) +
            ImGui::Button("Regenerate Noise Texture"))
        {
            stage.regenerate_noise_texture(noise_resolution);
        }
    }

    if (ImGui::TreeNode("Debug"))
    {
        DEFER(ImGui::TreePop());
        const auto sampler = [](const char* name, RawSampler<> sampler)
        {
            ImGui::PushID(name);
            DEFER(ImGui::PopID());
            ImGui::SeparatorText(name);
            {
                MinFilter minfilter = sampler.get_min_filter();
                MagFilter magfilter = sampler.get_mag_filter();
                Wrap      wrap      = sampler.get_wrap_s();
                bool changed = false;
                changed |= ImGui::EnumCombo("Min Filter", &minfilter);
                changed |= ImGui::EnumCombo("Mag Filter", &magfilter);
                changed |= ImGui::EnumCombo("Wrap",       &wrap);
                if (changed)
                {
                    sampler.set_min_mag_filters(minfilter, magfilter);
                    sampler.set_wrap_all(wrap);
                }
            }
        };
        const auto texture = [](const char* name, RawTexture2D<> texture)
        {
            ImGui::PushID(name);
            DEFER(ImGui::PopID());
            ImGui::SeparatorText(name);
            {
                MinFilter minfilter = texture.get_sampler_min_filter();
                MagFilter magfilter = texture.get_sampler_mag_filter();
                Wrap      wrap      = texture.get_sampler_wrap_s();
                bool changed = false;
                changed |= ImGui::EnumCombo("Min Filter", &minfilter);
                changed |= ImGui::EnumCombo("Mag Filter", &magfilter);
                changed |= ImGui::EnumCombo("Wrap",       &wrap);
                if (changed)
                {
                    texture.set_sampler_min_mag_filters(minfilter, magfilter);
                    texture.set_sampler_wrap_all(wrap);
                }
            }
        };
        sampler("Depth Sampler",   stage._depth_sampler);
        sampler("Normals Sampler", stage._normals_sampler);
        sampler("Blur Sampler",    stage._blur_sampler);
        texture("Noise Texture",   stage._noise_texture);
    }
}
