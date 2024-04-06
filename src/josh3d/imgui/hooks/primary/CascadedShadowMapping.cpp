#include "detail/SimpleStageHookMacro.hpp"
#include "CascadedShadowMapping.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, CascadedShadowMapping) {

    ImGui::Checkbox("Backface Culling", &stage_.enable_backface_culling);

}
