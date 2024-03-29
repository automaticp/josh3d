#include "ImGuiWindowSettings.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>


namespace josh {


void ImGuiWindowSettings::display() {

    // All the monitor handling is done through C glfw api;
    // the glfwpp is somewhat shaky with it's monitor handling.

    GLFWmonitor* primary_monitor =
        glfwGetPrimaryMonitor();

    if (primary_monitor) {
        ImGui::Text(
            "Primary Monitor: %s",
            glfwGetMonitorName(primary_monitor)
        );
    }

    if (ImGui::Checkbox("V-Sync", &is_vsync_on_)) {
        glfw::swapInterval(is_vsync_on_ ? 1 : 0);
    }

    if (ImGui::Checkbox("Fullscreen", &is_fullscreen_)) {

        if (primary_monitor && is_fullscreen_) {

            const GLFWvidmode* mode =
                glfwGetVideoMode(primary_monitor);

            windowed_params_ = get_current_windowed_params();

            glfwSetWindowMonitor(
                window_, primary_monitor, 0, 0,
                mode->width, mode->height, mode->refreshRate
            );

        } else /* switched back to windowed */ {

            // Restore windowed-mode params.
            auto [x, y, w, h] = windowed_params_;

            glfwSetWindowMonitor(
                window_, nullptr, x, y, w, h, 0
            );

        }

    }

}





} // namespace josh
