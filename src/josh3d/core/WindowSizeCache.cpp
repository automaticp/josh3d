#include "WindowSizeCache.hpp"
#include <glfwpp/window.h>
#include <cassert>


namespace josh {


void WindowSizeCache::track(glfw::Window& window) {
    window_ = &window;
    update_from_tracked();
}


void WindowSizeCache::update_from_tracked() {
    assert(window_);
    size_ = std::make_from_tuple<Size2I>(window_->getSize());
}


} // namespace josh
