#pragma once
#include "Common.hpp"
#include "Runtime.hpp"
#include "ImGuiContextWrapper.hpp"
#include "ImGuiResourceViewer.hpp"
#include "ImGuiSceneList.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuiWindowSettings.hpp"
#include "ImGuiVFSControl.hpp"
#include "ImGuiEngineHooks.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuizmoGizmos.hpp"
#include "AvgFrameTimeCounter.hpp"
#include <sstream>


namespace glfw { class Window; }


namespace josh {


struct ImGuiIOWants
{
    bool capture_mouse;
    bool capture_mouse_unless_popup_close;
    bool capture_keyboard;
    bool text_input;
    bool set_mouse_pos;
    bool save_ini_settings;
};

/*
Application-wide assembly of different windows and widgets.

Your UI entrypoint.
*/
struct ImGuiApplicationAssembly
{
    bool hidden               = false;
    bool show_engine_hooks    = true;
    bool show_scene_list      = true;
    bool show_selected        = true;
    bool show_demo_window     = false; // For debugging.
    bool show_asset_manager   = false; // For debugging.
    bool show_resource_viewer = false;
    bool show_frame_graph     = false;
    bool show_debug_window    = false; // General debugging stuff.

    float background_alpha = 0.8f;

    ImGuiApplicationAssembly(
        glfw::Window& window,
        Runtime&      runtime);

    void new_frame(const FrameTimer& frame_timer);
    void display();

    auto get_log_sink() noexcept -> std::ostream& { return _log_sink; }
    auto get_io_wants() const noexcept -> ImGuiIOWants;

    glfw::Window& window;
    Runtime&      runtime;

    ImGuiContextWrapper imgui_context;
    ImGuiWindowSettings window_settings;
    ImGuiVFSControl     vfs_control;
    ImGuiEngineHooks    stage_hooks;
    ImGuiSceneList      scene_list;
    ImGuiResourceViewer resource_viewer;
    ImGuiSelected       selected_menu;
    ImGuizmoGizmos      gizmos;

    std::ostringstream _log_sink; // Why am I owning this sink?
    usize              _last_log_size = {};

    AvgFrameTimeCounter _avg_frame_timer = { 0.500f };
    // Well, lets hope the FPS doesn't exceed 99k.
    // It will just not display properly if it does, no UB.
    static constexpr const char* _fps_str_template = "FPS: xxxxx.x";
    static constexpr const char* _fps_str_fmt      = "FPS: %.1f";

    static constexpr const char* _frametime_str_template = "Frametime: xxxxx.xms";
    static constexpr const char* _frametime_str_fmt      = "Frametime: %.1fms";

    static constexpr const char* _gizmo_info_str_template = "Gizmo: xx   ";
    static constexpr const char* _gizmo_info_str_fmt      = "Gizmo: %c%c   ";

    String _fps_str        = _fps_str_template;
    String _frametime_str  = _frametime_str_template;
    String _gizmo_info_str = _gizmo_info_str_template;

    void _draw_widgets();
    void _reset_dockspace(unsigned dockspace_id);

    // FrameGraph widget state. TODO: Move elsewhere?
    int           _num_frames_plotted    = 300;
    int           _frame_offset          = {};
    float         _upper_frametime_limit = 33.f;
    Vector<float> _frame_deltas;

    void _display_frame_graph();
    void _display_debug();
};


} // namespace josh
