#include "ImGuiWindowSettings.hpp"
#include "GlobalsUtil.hpp"
#include <imgui.h>
#include <cstdio>
#include <string>


namespace josh {


void ImGuiWindowSettings::display() {

    if (hidden) { return; }

    avg_frame_timer_.update(globals::frame_timer.delta<float>());

    ImGui::SetNextWindowSize({ 400.f, 400.f }, ImGuiCond_Once);
    ImGui::SetNextWindowPos({ 600.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);

    std::snprintf(
        title_, title_buf_size,
        "Window Settings / FPS: %.1f###WindowSettings",
        1.f / avg_frame_timer_.get_current_average()
    );

    if (ImGui::Begin(title_)) {

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

        ImGui::SliderFloat(
            "FPS Averaging Interval, s",
            &avg_frame_timer_.averaging_interval,
            0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );


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

    } ImGui::End();

}





} // namespace josh
