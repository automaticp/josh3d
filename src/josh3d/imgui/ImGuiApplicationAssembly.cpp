#include "ImGuiApplicationAssembly.hpp"
#include "FrameTimer.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuizmoGizmos.hpp"
#include "PerspectiveCamera.hpp"
#include "RenderEngine.hpp"
#include "Size.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstdio>




namespace josh {


ImGuiApplicationAssembly::ImGuiApplicationAssembly(
    glfw::Window& window,
    RenderEngine& engine,
    entt::registry& registry,
    const PerspectiveCamera& cam,
    VirtualFilesystem& vfs
)
    : context_{ window }
    , window_settings_{ window }
    , vfs_control_{ vfs }
    , stage_hooks_{ engine }
    , registry_hooks_{ registry }
    , selected_menu_{ registry }
    , gizmos_{ cam, registry }
{}


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
        fps_str_.data(), fps_str_.size() + 1,
        fps_str_fmt_,
        1.f / avg_frame_timer_.get_current_average()
    );

    std::snprintf(
        frametime_str_.data(), frametime_str_.size() + 1,
        frametime_str_fmt_,
        avg_frame_timer_.get_current_average() * 1.E3f // s -> ms
    );

    std::snprintf(
        gizmo_info_str_.data(), gizmo_info_str_.size() + 1,
        gizmo_info_str_fmt_,
        [this]() -> char {
            switch (active_gizmo_space()) {
                using enum GizmoSpace;
                case world: return 'W';
                case local: return 'L';
                default:    return ' ';
            }
        }(),
        [this]() -> char {
            switch (active_gizmo_operation()) {
                using enum GizmoOperation;
                case translation: return 'T';
                case rotation:    return 'R';
                case scaling:     return 'S';
                default:          return ' ';
            }
        }()
    );

    context_.new_frame();
    gizmos_.new_frame();
}


void ImGuiApplicationAssembly::draw_widgets() {

    // TODO: Keep active windows within docknodes across "hides".

    auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    bg_col.w = 0.f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);
    // FIXME: This is probably terribly broken in some way.
    // For the frames that reset the dockspace, this initialization
    // may be reset before OR after the widgets are drawn:
    // in the direct call to reset_dockspace() on resize,
    // or on_value_change_from() after the widget scope.
    // How it still works somewhat "correctly" after both,
    // is beyond me.
    ImGuiID dockspace_id =
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
    ImGui::PopStyleColor();

    // FIXME: Terrible, maybe will add "was resized" flag to WindowSizeCache instead.
    static Size2F old_size{ 0, 0 };
    auto vport_size = ImGui::GetMainViewport()->Size;
    Size2F new_size = { vport_size.x, vport_size.y };
    if (old_size != new_size) {
        // Do the reset inplace, before the windows are submitted.
        reset_dockspace(dockspace_id);
        old_size = new_size;
    }

    if (!is_hidden()) {

        auto reset_condition = on_value_change_from(false, [&, this] { reset_dockspace(dockspace_id); });

        auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        bg_col.w = background_alpha;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);

        if (ImGui::BeginMainMenuBar()) {
            ImGui::TextUnformatted("Josh3D-Demo");

            const float size_gizmo     = ImGui::CalcTextSize(gizmo_info_str_template_).x;
            const float size_fps       = ImGui::CalcTextSize(fps_str_template_).x;
            const float size_frametime = ImGui::CalcTextSize(frametime_str_template_).x;

            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_gizmo + size_fps + size_frametime));
            ImGui::TextUnformatted(gizmo_info_str_.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_fps + size_frametime));
            ImGui::TextUnformatted(fps_str_.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_frametime));
            ImGui::TextUnformatted(frametime_str_.c_str());

            ImGui::EndMainMenuBar();
        }

        if (ImGui::Begin("ImGui")) {

            ImGui::SliderFloat(
                "FPS Avg. Interval, s", &avg_frame_timer_.averaging_interval,
                0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat("Bg. Alpha", &background_alpha, 0.f, 1.f);

            reset_condition.set(ImGui::Button("Reset Dockspace"));

        } ImGui::End();

        if (ImGui::Begin("Window")) {
            window_settings_.display();
        } ImGui::End();

        if (ImGui::Begin("Render Engine")) {
            stage_hooks_.display();
        } ImGui::End();


        if (ImGui::Begin("VFS")) {
            vfs_control_.display();
        } ImGui::End();

        if (ImGui::Begin("Selected")) {
            selected_menu_.display();
        } ImGui::End();

        if (ImGui::Begin("Registry")) {
            registry_hooks_ .display();
        } ImGui::End();

        ImGui::PopStyleColor();
    }

}


void ImGuiApplicationAssembly::display() {
    draw_widgets();
    gizmos_.display();
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

    ImGui::DockBuilderDockWindow("Registry", left_id);
    ImGui::DockBuilderDockWindow("Selected", left_id);
    ImGui::DockBuilderDockWindow("VFS",      left_id);
    ImGui::DockBuilderDockWindow("Render Engine", right_id);
    ImGui::DockBuilderDockWindow("Window",        right_id);
    ImGui::DockBuilderDockWindow("ImGui",         right_id);

    ImGui::DockBuilderFinish(dockspace_id);
}


} // namespace josh
