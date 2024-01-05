#pragma once
#include "ImGuiContextWrapper.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuiWindowSettings.hpp"
#include "ImGuiVFSControl.hpp"
#include "ImGuiStageHooks.hpp"
#include "ImGuiRegistryHooks.hpp"
#include "ImGuiSelected.hpp"
#include "AvgFrameTimeCounter.hpp"
#include <cstring>
#include <string>
#include <entt/fwd.hpp>


namespace glfw { class Window; }


namespace josh {


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
    ImGuiContextWrapper context_;
    ImGuiWindowSettings window_settings_;
    ImGuiVFSControl     vfs_control_;
    ImGuiStageHooks     stage_hooks_;
    ImGuiRegistryHooks  registry_hooks_;
    ImGuiSelected       selected_menu_;

    AvgFrameTimeCounter avg_frame_timer_{ 0.500f };
    // Well, lets hope the FPS doesn't exceed 99k.
    // It will just not display properly if it does, no UB.
    static constexpr const char* fps_str_template_{ "FPS: xxxxx.x" };
    static constexpr const char* fps_str_fmt_     { "FPS: %.1f"    };

    static constexpr const char* frametime_str_template_{ "Frametime: xxxxx.xms" };
    static constexpr const char* frametime_str_fmt_     { "Frametime: %.1fms"    };

    std::string fps_str_      { fps_str_template_       };
    std::string frametime_str_{ frametime_str_template_ };

    bool hidden_{ false };

    void draw_widgets();
    void reset_dockspace(unsigned int dockspace_id);

public:
    float background_alpha{ 0.8f };

    ImGuiApplicationAssembly(
        glfw::Window& window, entt::registry& registry, VirtualFilesystem& vfs);

    ImGuiStageHooks::HooksContainer&       stage_hooks() noexcept { return stage_hooks_.hooks(); }
    const ImGuiStageHooks::HooksContainer& stage_hooks() const noexcept { return stage_hooks_.hooks(); }

    ImGuiRegistryHooks::HooksContainer&       registry_hooks() noexcept { return registry_hooks_.hooks(); }
    const ImGuiRegistryHooks::HooksContainer& registry_hooks() const noexcept { return registry_hooks_.hooks(); }

    bool is_hidden() const noexcept { return hidden_; }
    void set_hidden(bool hidden) noexcept { hidden_ = hidden; }
    void toggle_hidden() noexcept { set_hidden(!is_hidden()); }

    void new_frame();
    void display();

    ImGuiIOWants get_io_wants() const noexcept;

};


} // namespace josh
