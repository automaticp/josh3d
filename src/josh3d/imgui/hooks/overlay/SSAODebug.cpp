#include "SSAODebug.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/overlay/SSAODebug.hpp"
#include "EnumUtils.hpp"
#include <imgui.h>
#include <iterator>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, SSAODebug) {

    const char* mode_names[] = {
        "None",
        "Noisy Occlusion",
        "Occlusion",
    };

    using Mode = stages::overlay::SSAODebug::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 3))
    {
        stage_.mode = Mode{ mode_id };
    }

}
