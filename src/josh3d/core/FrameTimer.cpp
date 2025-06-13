#include "FrameTimer.hpp"
#include <glfwpp/glfwpp.h>


namespace josh {

void FrameTimer::update() noexcept
{
    previous_ = current_;
    // FIXME: This is not the most reliable source of timing information.
    current_ = glfw::getTime();
    delta_ = current_ - previous_;
}

} // namespace josh
