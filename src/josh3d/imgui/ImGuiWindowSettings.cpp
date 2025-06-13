#include "ImGuiWindowSettings.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>


namespace josh {


void ImGuiWindowSettings::display()
{
    // NOTE: All the monitor handling is done through C glfw api;
    // the glfwpp is somewhat shaky with it's monitor handling.

    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

    if (primary_monitor)
    {
        ImGui::Text("Primary Monitor: %s",
            glfwGetMonitorName(primary_monitor));
    }

    if (ImGui::Checkbox("V-Sync", &_is_vsync_on))
        glfw::swapInterval(_is_vsync_on ? 1 : 0);

    if (ImGui::Checkbox("Fullscreen", &_is_fullscreen))
    {
        if (primary_monitor and _is_fullscreen)
        {
            const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

            _windowed_params = _get_current_windowed_params();

            glfwSetWindowMonitor(window, primary_monitor, 0, 0,
                mode->width, mode->height, mode->refreshRate);

        }
        else /* switched back to windowed */
        {
            // Restore windowed-mode params.
            auto [x, y, w, h] = _windowed_params;
            glfwSetWindowMonitor(window, nullptr, x, y, w, h, 0);
        }
    }
}


} // namespace josh
