#pragma once
#include "GLFW/glfw3.h"
#include <glfwpp/window.h>
#include <glfwpp/monitor.h>
#include <glfwpp/glfwpp.h>


namespace josh {

struct ImGuiWindowSettings
{
    glfw::Window& window;

    void display();

    bool _is_vsync_on   = false; // FIXME: assumed, not guaranteed
    bool _is_fullscreen = bool((GLFWmonitor*)window.getMonitor());

    struct WindowedParamsBackup
    {
        int xpos, ypos, width, height;
    };

    // Saved before going fullscreen.
    WindowedParamsBackup _windowed_params = _get_current_windowed_params();

    auto _get_current_windowed_params() const
        -> WindowedParamsBackup
    {
        auto [w, h] = window.getSize();
        auto [x, y] = window.getPos();
        return { x, y, w, h };
    }
};

} // namespace josh
