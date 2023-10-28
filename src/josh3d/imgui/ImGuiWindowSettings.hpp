#pragma once
#include "GLFW/glfw3.h"
#include <glfwpp/window.h>
#include <glfwpp/monitor.h>
#include <glfwpp/glfwpp.h>



namespace josh {



class ImGuiWindowSettings {
private:
    glfw::Window& window_;

    bool is_vsync_on_{ false }; // FIXME: assumed, not guaranteed
    bool is_fullscreen_{
        bool(static_cast<GLFWmonitor*>(window_.getMonitor()))
    };


    struct WindowedParamsBackup {
        int xpos, ypos, width, height;
    };

    // Saved before going fullscreen.
    WindowedParamsBackup windowed_params_{
        get_current_windowed_params()
    };

    WindowedParamsBackup get_current_windowed_params() const {
        auto [w, h] = window_.getSize();
        auto [x, y] = window_.getPos();
        return { x, y, w, h };
    }

public:
    ImGuiWindowSettings(glfw::Window& window) : window_{ window } {}

    void display();
};





} // namespace josh
