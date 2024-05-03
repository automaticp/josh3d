#include "detail/SimpleStageHookMacro.hpp"
#include "PointShadowMapping.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, PointShadowMapping) {

    auto& maps = stage_.view_output().point_shadow_maps_tgt;

    int resolution = maps.resolution().width;
    if (ImGui::SliderInt("Resolution", &resolution,
        128, 8192, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.resize_maps(Size2I{ resolution, resolution });
    }

}

