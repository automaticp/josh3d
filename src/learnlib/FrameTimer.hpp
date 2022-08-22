#pragma once
#include <glfwpp/glfwpp.h>


namespace learn {


class FrameTimer {
private:
    double current_{};
    double previous_{};
    double delta_{};

public:
    void update() noexcept {
        current_ = glfw::getTime();
        delta_ = current_ - previous_;
        previous_ = current_;
    }

    double current() const noexcept { return current_; }
    double previous() const noexcept { return previous_; }
    double delta() const noexcept { return delta_; }
};



inline FrameTimer global_frame_timer{};


} // namespace learn
