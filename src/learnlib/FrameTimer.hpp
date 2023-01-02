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

    template<typename FloatT = double>
    FloatT current() const noexcept { return static_cast<FloatT>(current_); }
    template<typename FloatT = double>
    FloatT previous() const noexcept { return static_cast<FloatT>(previous_); }
    template<typename FloatT = double>
    FloatT delta() const noexcept { return static_cast<FloatT>(delta_); }
};



inline FrameTimer global_frame_timer{};


} // namespace learn
