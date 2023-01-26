#pragma once


namespace glfw { class Window; }

namespace learn {


class KeyCallbackArgs;
class CursorPosCallbackArgs;
class ScrollCallbackArgs;


class ImGuiInputBlocker {
public:
    bool is_key_blocked(const KeyCallbackArgs&) const noexcept;
    bool is_cursor_blocked(const CursorPosCallbackArgs&) const noexcept;
    bool is_scroll_blocked(const ScrollCallbackArgs&) const noexcept;
};


class ImGuiContextWrapper {
public:
    ImGuiContextWrapper(glfw::Window& window);

    void new_frame() const;

    void render() const;

    ImGuiContextWrapper(const ImGuiContextWrapper&) = delete;
    ImGuiContextWrapper& operator=(const ImGuiContextWrapper&) = delete;

    ~ImGuiContextWrapper() noexcept;
};


} // namespace learn
