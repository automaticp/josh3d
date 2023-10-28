#include "ImGuiApplicationAssembly.hpp"
#include "FrameTimer.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstdio>
#include <iterator>


namespace josh {


ImGuiIOWants ImGuiApplicationAssembly::get_io_wants() const noexcept {
    auto& io = ImGui::GetIO();

    return {
        .capture_mouse     = io.WantCaptureMouse,
        .capture_mouse_unless_popup_close
                           = io.WantCaptureMouseUnlessPopupClose,
        .capture_keyboard  = io.WantCaptureKeyboard,
        .text_input        = io.WantTextInput,
        .set_mouse_pos     = io.WantSetMousePos,
        .save_ini_settings = io.WantSaveIniSettings
    };
}


void ImGuiApplicationAssembly::new_frame() {
    // FIXME: Use external FrameTimer.
    avg_frame_timer_.update(globals::frame_timer.delta<float>());

    std::snprintf(
        fps_str_.data(), fps_str_.size(),
        fps_str_fmt_,
        1.f / avg_frame_timer_.get_current_average()
    );

    std::snprintf(
        frametime_str_.data(), frametime_str_.size(),
        frametime_str_fmt_,
        avg_frame_timer_.get_current_average() * 1.E3f // s -> ms
    );

    context_.new_frame();
}


void ImGuiApplicationAssembly::display() {

    // TODO: Keep active windows within docknodes across "hides".
    // TODO: Autodock on initialization.

    auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    bg_col.w = 0.f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);
    ImGuiID dockspace_id =
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
    ImGui::PopStyleColor();

    if (!is_hidden()) {
        auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        bg_col.w = background_alpha;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);

        if (ImGui::BeginMainMenuBar()) {
            ImGui::TextUnformatted("Josh3D-Demo");

            const float size_fps       = ImGui::CalcTextSize(fps_str_template_).x;
            const float size_frametime = ImGui::CalcTextSize(frametime_str_template_).x;
            const float offset = size_fps + size_frametime;
            ImGui::SameLine(ImGui::GetContentRegionMax().x - offset);
            ImGui::TextUnformatted(fps_str_.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - size_frametime);
            ImGui::TextUnformatted(frametime_str_.c_str());

            ImGui::EndMainMenuBar();
        }

        if (ImGui::Begin("ImGui")) {

            ImGui::SliderFloat(
                "FPS Avg. Interval, s", &avg_frame_timer_.averaging_interval,
                0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat("Bg. Alpha", &background_alpha, 0.f, 1.f);

            if (ImGui::Button("Reset Dockspace")) {
                reset_dockspace(dockspace_id);
            }

        } ImGui::End();

        if (ImGui::Begin("VFS")) {
            vfs_control_.display();
        } ImGui::End();

        if (ImGui::Begin("Window")) {
            window_settings_.display();
        } ImGui::End();

        if (ImGui::Begin("Render Stages")) {
            stage_hooks_.display();
        } ImGui::End();

        if (ImGui::Begin("Registry")) {
            registry_hooks_ .display();
        } ImGui::End();

        ImGui::PopStyleColor();
    }

    context_.render();
}


void ImGuiApplicationAssembly::reset_dockspace(ImGuiID dockspace_id) {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    auto flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags{ ImGuiDockNodeFlags_DockSpace };
	ImGui::DockBuilderAddNode(dockspace_id, flags);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    float h_split = 3.5f;
    auto left_id  = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left,  1.f / h_split, nullptr, &dockspace_id);
    h_split -= 1.f;
    auto right_id = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 1.f / h_split, nullptr, &dockspace_id);

    auto right_top_id = ImGui::DockBuilderSplitNode(right_id, ImGuiDir_Up,    1.f / 6.f, nullptr, &right_id);

    ImGui::DockBuilderDockWindow("VFS",      left_id);
    ImGui::DockBuilderDockWindow("Registry", left_id);
    ImGui::DockBuilderDockWindow("Render Stages", right_id);
    ImGui::DockBuilderDockWindow("Window",        right_top_id);
    ImGui::DockBuilderDockWindow("ImGui",         right_top_id);

    ImGui::DockBuilderFinish(dockspace_id);
}


} // namespace josh
