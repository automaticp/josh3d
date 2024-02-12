#include "SSAO.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, SSAO) {

    int kernel_size = static_cast<int>(stage_.get_kernel_size());

    ImGui::Checkbox("Enable Occlusion Sampling", &stage_.enable_occlusion_sampling);

    if (ImGui::SliderInt("Kernel Size", &kernel_size,
        1, 256, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.set_kernel_size(static_cast<size_t>(kernel_size));
    }

    ImGui::SliderFloat(
        "Radius", &stage_.radius, 0.001f, 1000.f,
        "%.3f", ImGuiSliderFlags_Logarithmic
    );

}
