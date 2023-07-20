#include "ImGuiInputBlocker.hpp"
#include <imgui.h>

namespace josh {


bool ImGuiInputBlocker::is_key_blocked(const KeyCallbackArgs&) const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiInputBlocker::is_cursor_blocked(const CursorPosCallbackArgs&) const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiInputBlocker::is_scroll_blocked(const ScrollCallbackArgs&) const {
    return ImGui::GetIO().WantCaptureMouse;
}


} // namespace josh
