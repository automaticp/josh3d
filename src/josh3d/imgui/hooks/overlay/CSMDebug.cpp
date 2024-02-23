#include "CSMDebug.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/overlay/CSMDebug.hpp"
#include "EnumUtils.hpp"
#include <imgui.h>
#include <iterator>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(overlay, CSMDebug) {

    const char* mode_names[] = {
        "None",
        "Views",
        "Maps",
    };

    using Mode = stages::overlay::CSMDebug::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 3))
    {
        stage_.mode = Mode{ mode_id };
    }


    if (stage_.mode == Mode::maps) {

        int max_cascade_id = static_cast<int>(stage_.num_cascades()) - 1;
        int cascade_id = static_cast<int>(stage_.cascade_id);
        if (ImGui::SliderInt("Cascade ID", &cascade_id, 0, max_cascade_id)) {
            stage_.cascade_id = cascade_id;
        }

    }

}
