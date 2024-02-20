#include "DeferredGeometry.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, DeferredGeometry) {

    ImGui::Checkbox("Backface Culling", &stage_.enable_backface_culling);

}
