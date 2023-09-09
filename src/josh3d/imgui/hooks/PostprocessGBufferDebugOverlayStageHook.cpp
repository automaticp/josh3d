#include "PostprocessGBufferDebugOverlayStageHook.hpp"
#include "stages/PostprocessGBufferDebugOverlayStage.hpp"
#include "EnumUtils.hpp"
#include <imgui.h>
#include <iterator>



void josh::imguihooks::PostprocessGBufferDebugOverlayStageHook::operator()() {

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

    using Mode = PostprocessGBufferDebugOverlayStage::OverlayMode;

    int mode_id = to_underlying(stage_.mode);
    if (ImGui::ListBox("Overlay", &mode_id,
            mode_names, std::size(mode_names), 5))
    {
        stage_.mode = Mode{ mode_id };
    }


}
