#include "detail/SimpleStageHookMacro.hpp"
#include "PointShadowMapping.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, PointShadowMapping) {

    auto& maps = stage_.view_output().point_shadow_maps_tgt;

    int resolution = maps.resolution().width;
    if (ImGui::SliderInt("New Resolution", &resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.resize_maps(Size2I{ resolution, resolution });
    }

    ImGui::DragFloat2(
        "Z Near/Far", glm::value_ptr(stage_.z_near_far()),
        1.0f, 0.001f, 1e4f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

}

