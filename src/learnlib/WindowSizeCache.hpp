#pragma once
#include <glfwpp/window.h>
#include <cassert>
#include <tuple>


namespace learn {

// This exists because the normal glfw::Window::getSize() call
// involves an expensive syscall. And even though you'd be tempted
// to get the size of the widnow via that method everytime you need it,
// it's much better if it's stored somewhere in the program memory and
// updated only on resize callbacks.
//
// TODO: extend to framebuffer size maybe


class WindowSizeCache {
private:
    glfw::Window* window_{ nullptr };
    int width_;
    int height_;

public:
    // Doesn't track any window by default.
    // Call track() to begin tracking a particular window.
    WindowSizeCache() = default;

    // Begins tracking a particular window to update the size from.
    // Initializes the size by calling getSize() as well.
    void track(glfw::Window& window) {
        window_ = &window;
        update_from_tracked();
    }

    // Updates the window size by calling getSize() on the tracked window.
    // UB if no window is tracked.
    // Either call this once on every frame, or update manually
    // only on resize events in callbacks using set_to().
    //
    // Prefer using set_to() whenever possible.
    void update_from_tracked() {
        assert(window_);
        auto [w, h] = window_->getSize();
        width_ = w;
        height_ = h;
    }

    // Manually sets the size of the window.
    // Can be used within window size or framebuffer size callbacks.
    void set_to(int width, int height) noexcept {
        width_ = width;
        height_ = height;
    }

    std::tuple<int, int> size() const noexcept {
        return { width_, height_ };
    }

    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }

};


} // namespace learn
