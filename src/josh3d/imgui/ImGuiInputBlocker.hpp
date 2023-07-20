#pragma once
#include "Input.hpp"


namespace josh {


class ImGuiInputBlocker : public IInputBlocker {
public:
    bool is_key_blocked(const KeyCallbackArgs&) const override;
    bool is_cursor_blocked(const CursorPosCallbackArgs&) const override;
    bool is_scroll_blocked(const ScrollCallbackArgs&) const override;
};


} // namespace josh
