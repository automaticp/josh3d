#include "ImGuiApplicationAssembly.hpp"
#include <imgui.h>


namespace josh {


ImGuiIOWants ImGuiApplicationAssembly::get_io_wants() const noexcept {
    auto& io = ImGui::GetIO();

    return {
        .capture_mouse     = io.WantCaptureMouse,
        .capture_mouse_unless_popup_close
                           = io.WantCaptureMouseUnlessPopupClose,
        .capture_keyboard  = io.WantCaptureKeyboard,
        .text_input        = io.WantTextInput,
        .set_mouse_pos     = io.WantSetMousePos,
        .save_ini_settings = io.WantSaveIniSettings
    };
}








} // namespace josh
