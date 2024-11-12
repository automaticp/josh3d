#pragma once
#include "ImGuiContextWrapper.hpp"
#include "ImGuiSceneList.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuiWindowSettings.hpp"
#include "ImGuiVFSControl.hpp"
#include "ImGuiEngineHooks.hpp"
#include "ImGuiSelected.hpp"
#include "AvgFrameTimeCounter.hpp"
#include "ImGuizmoGizmos.hpp"
#include "AssetImporter.hpp"
#include <sstream>
#include <string>
#include <entt/fwd.hpp>


namespace glfw { class Window; }


namespace josh {


class RenderEngine;


struct ImGuiIOWants {
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
class ImGuiApplicationAssembly {
private:
    glfw::Window&            window_;
    RenderEngine&            engine_;
    entt::registry&          registry_;
    AssetImporter&           asset_importer_;
    VirtualFilesystem&       vfs_;

    ImGuiContextWrapper context_;
    ImGuiWindowSettings window_settings_;
    ImGuiVFSControl     vfs_control_;
    ImGuiEngineHooks    stage_hooks_;
    ImGuiSceneList      scene_list_;
    ImGuiSelected       selected_menu_;
    ImGuizmoGizmos      gizmos_;

    std::ostringstream log_sink_;
    size_t             last_log_size_{};

    AvgFrameTimeCounter avg_frame_timer_{ 0.500f };
    // Well, lets hope the FPS doesn't exceed 99k.
    // It will just not display properly if it does, no UB.
    static constexpr const char* fps_str_template_{ "FPS: xxxxx.x" };
    static constexpr const char* fps_str_fmt_     { "FPS: %.1f"    };

    static constexpr const char* frametime_str_template_{ "Frametime: xxxxx.xms" };
    static constexpr const char* frametime_str_fmt_     { "Frametime: %.1fms"    };

    static constexpr const char* gizmo_info_str_template_{ "Gizmo: xx   "   };
    static constexpr const char* gizmo_info_str_fmt_     { "Gizmo: %c%c   " };

    std::string fps_str_       { fps_str_template_        };
    std::string frametime_str_ { frametime_str_template_  };
    std::string gizmo_info_str_{ gizmo_info_str_template_ };

    bool hidden_{ false };

    void draw_widgets();
    void reset_dockspace(unsigned int dockspace_id);

public:
    float background_alpha{ 0.8f };

    ImGuiApplicationAssembly(
        glfw::Window&      window,
        RenderEngine&      engine,
        entt::registry&    registry,
        AssetImporter&     asset_importer,
        SceneImporter&     scene_importer,
        VirtualFilesystem& vfs);


    auto stage_hooks()       noexcept ->       ImGuiEngineHooks::HooksContainer& { return stage_hooks_.hooks(); }
    auto stage_hooks() const noexcept -> const ImGuiEngineHooks::HooksContainer& { return stage_hooks_.hooks(); }

    bool is_hidden() const noexcept { return hidden_; }
    void set_hidden(bool hidden) noexcept { hidden_ = hidden; }
    void toggle_hidden() noexcept { set_hidden(!is_hidden()); }

    void new_frame();
    void display();

    auto get_log_sink() -> std::ostream& { return log_sink_; }

    auto active_gizmo_operation()       noexcept ->       GizmoOperation& { return gizmos_.active_operation; }
    auto active_gizmo_operation() const noexcept -> const GizmoOperation& { return gizmos_.active_operation; }

    auto active_gizmo_space()       noexcept ->       GizmoSpace& { return gizmos_.active_space; }
    auto active_gizmo_space() const noexcept -> const GizmoSpace& { return gizmos_.active_space; }

    ImGuiIOWants get_io_wants() const noexcept;

};


} // namespace josh
