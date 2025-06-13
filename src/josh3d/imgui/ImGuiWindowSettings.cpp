#include "ImGuiWindowSettings.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "UIContext.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glfwpp/window.h>


namespace josh {
namespace {

auto get_windowed_params(const glfw::Window& window)
    -> ImGuiWindowSettings::WindowedParams
{
    auto [w, h] = window.getSize();
    auto [x, y] = window.getPos();
    return { x, y, w, h };
}

} // namespace

void ImGuiWindowSettings::display(UIContext& ui)
{
    // NOTE: All the monitor handling is done through C glfw api;
    // the glfwpp is somewhat shaky with it's monitor handling.

    GLFWmonitor* current_monitor = glfwGetWindowMonitor(ui.window);

    int num_monitors;
    GLFWmonitor** monitors = glfwGetMonitors(&num_monitors);

    for (const uindex i : irange(num_monitors))
    {
        ImGui::PushID(int(i));
        GLFWmonitor* monitor = monitors[i];
        const bool was_fullscreen = (monitor == current_monitor);
        bool       is_fullscreen  = was_fullscreen;
        ImGui::Text("Monitor %zu", i);
        ImGui::SameLine(); // FIXME: This is ugly.
        const bool clicked = ImGui::Checkbox("Fullscreen", &is_fullscreen);
        ImGui::SameLine();
        ImGui::TextUnformatted(glfwGetMonitorName(monitor));
        if (clicked)
        {
            if (not was_fullscreen and is_fullscreen)
            {
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);

                // HMM: It seems like this ignores decorations?
                _last_params = get_windowed_params(ui.window);

                // NOTE: Must be called from main thread only.
                // Maybe just move it to a dedicated window class?
                glfwSetWindowMonitor(ui.window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else if (was_fullscreen and is_fullscreen)
            {
                // This almost doesn't work. Feels very buggy. Why?
                const GLFWvidmode* mode = glfwGetVideoMode(current_monitor);
                glfwSetWindowMonitor(ui.window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else if (was_fullscreen and not is_fullscreen)
            {
                // Resore windowed state.
                const auto [x, y, w, h] = _last_params;
                glfwSetWindowMonitor(ui.window, nullptr, x, y, w, h, 0);
            }
            else
            {
                assert(false);
            }
        }
        ImGui::PopID();
    }

    if (ImGui::Checkbox("V-Sync", &_is_vsync_on))
        glfwSwapInterval(_is_vsync_on ? 1 : 0);
}


} // namespace josh
