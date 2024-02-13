#include "SSAO.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <glm/trigonometric.hpp>
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, SSAO) {

    ImGui::Checkbox("Enable Occlusion Sampling", &stage_.enable_occlusion_sampling);

    int kernel_size = static_cast<int>(stage_.get_kernel_size());
    if (ImGui::SliderInt("Kernel Size", &kernel_size,
        1, 256, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.set_kernel_size(static_cast<size_t>(kernel_size));
    }

    float min_angle_deg = glm::degrees(stage_.get_min_sample_angle_from_surface_rad());
    if (ImGui::SliderFloat("Min. Angle From Surface, Deg", &min_angle_deg,
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

}
