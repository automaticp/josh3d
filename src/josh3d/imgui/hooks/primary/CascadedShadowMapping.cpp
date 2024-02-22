#include "detail/SimpleStageHookMacro.hpp"
#include "CascadedShadowMapping.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, CascadedShadowMapping) {

    auto& maps = stage_.view_output().dir_shadow_maps_tgt.depth_attachment();

    int resolution = maps.size().width; // Keep Square.
    if (ImGui::SliderInt("New Resolution", &resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.resize_maps(Size2I{ resolution, resolution });
    }

}
