#include "OverlayGBufferDebugStageHook.hpp"
#include "stages/OverlayGBufferDebugStage.hpp"
#include "EnumUtils.hpp"
#include <imgui.h>
#include <iterator>



void josh::imguihooks::OverlayGBufferDebugStageHook::operator()() {

    const char* mode_names[] = {
        "None",
        "Albedo",
        "Specular",
        "Position",
        "Depth",
        "Depth (Linear)",
        "Normals",
        "Draw Region"
    };

    using Mode = OverlayGBufferDebugStage::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 5))
    {
        stage_.mode = Mode{ mode_id };
    }


}
