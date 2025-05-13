#pragma once
#include "AssetImporter.hpp"
#include "AssetUnpacker.hpp"
#include "AssetManager.hpp"
#include "ECS.hpp"
#include "ImGuiAssetBrowser.hpp"
#include "ImGuiContextWrapper.hpp"
#include "ImGuiResourceViewer.hpp"
#include "ImGuiSceneList.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuiWindowSettings.hpp"
#include "ImGuiVFSControl.hpp"
#include "ImGuiEngineHooks.hpp"
#include "ImGuiSelected.hpp"
#include "AvgFrameTimeCounter.hpp"
#include "ImGuizmoGizmos.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceUnpacker.hpp"
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
public:
    bool show_engine_hooks    = true;
    bool show_scene_list      = true;
    bool show_selected        = true;
    bool show_asset_browser   = false;
    bool show_demo_window     = false; // For debugging.
    bool show_asset_manager   = false; // For debugging.
    bool show_resource_viewer = false;

    float background_alpha{ 0.8f };

    ImGuiApplicationAssembly(
        glfw::Window&      window,
        RenderEngine&      engine,
        Registry&          registry,
        AssetManager&      asset_manager,
        AssetUnpacker&     asset_2npacker_,
        SceneImporter&     scene_importer,
        ResourceDatabase&  resource_database,
        AssetImporter&     asset_importer,
        ResourceUnpacker&  resource_unpacker,
        VirtualFilesystem& vfs);


    auto stage_hooks()       noexcept ->       ImGuiEngineHooks::HooksContainer& { return stage_hooks_.hooks(); }
    auto stage_hooks() const noexcept -> const ImGuiEngineHooks::HooksContainer& { return stage_hooks_.hooks(); }

    auto resource_viewer() noexcept -> ImGuiResourceViewer& { return resource_viewer_; }

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


private:

    glfw::Window&      window_;
    RenderEngine&      engine_;
    Registry&          registry_;
    AssetManager&      asset_manager_;
    AssetUnpacker&     asset_unpacker_;
    ResourceDatabase&  resource_database_;
    AssetImporter&     asset_importer_;
    ResourceUnpacker&  resource_unpacker_;
    VirtualFilesystem& vfs_;

    ImGuiContextWrapper context_;
    ImGuiWindowSettings window_settings_;
    ImGuiVFSControl     vfs_control_;
    ImGuiEngineHooks    stage_hooks_;
    ImGuiSceneList      scene_list_;
    ImGuiAssetBrowser   asset_browser_;
    ImGuiResourceViewer resource_viewer_;
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

};


} // namespace josh
