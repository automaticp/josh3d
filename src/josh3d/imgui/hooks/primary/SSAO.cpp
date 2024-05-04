#include "SSAO.hpp"
#include "EnumUtils.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/primary/SSAO.hpp"
#include <glm/trigonometric.hpp>
#include <imgui.h>


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
