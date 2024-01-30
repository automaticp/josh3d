#include "GBufferDebug.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/overlay/GBufferDebug.hpp"
#include "EnumUtils.hpp"
#include <imgui.h>
#include <iterator>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, GBufferDebug) {

    const char* mode_names[] = {
        "None",
        "Albedo",
        "Specular",
        "Position",
        "Depth",
        "Depth (Linear)",
        "Normals",
        "Draw Region",
        "Object ID"
    };

    using Mode = stages::overlay::GBufferDebug::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 5))
    {
        stage_.mode = Mode{ mode_id };
    }

}
