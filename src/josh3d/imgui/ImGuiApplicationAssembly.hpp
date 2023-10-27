#pragma once
#include "ImGuiContextWrapper.hpp"
#include "ImGuiWindowSettings.hpp"
#include "ImGuiVFSControl.hpp"
#include "ImGuiStageHooks.hpp"
#include "ImGuiRegistryHooks.hpp"
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

    bool hidden_{ false };

public:
    ImGuiApplicationAssembly(glfw::Window& window,
        entt::registry& registry, VirtualFilesystem& vfs)
        : context_{ window }
        , window_settings_{ window }
        , vfs_control_{ vfs }
        , stage_hooks_{}
        , registry_hooks_{ registry }
    {
        set_hidden(hidden_);
    }

    ImGuiStageHooks::HooksContainer&       stage_hooks() noexcept { return stage_hooks_.hooks(); }
    const ImGuiStageHooks::HooksContainer& stage_hooks() const noexcept { return stage_hooks_.hooks(); }

    ImGuiRegistryHooks::HooksContainer&       registry_hooks() noexcept { return registry_hooks_.hooks(); }
    const ImGuiRegistryHooks::HooksContainer& registry_hooks() const noexcept { return registry_hooks_.hooks(); }

    bool is_hidden() const noexcept { return hidden_; }
    void set_hidden(bool hidden) noexcept;
    void toggle_hidden() noexcept { set_hidden(!is_hidden()); }

    void new_frame() const noexcept { context_.new_frame(); }
    // TODO: Is there a better name than "display"?
    void display();

    ImGuiIOWants get_io_wants() const noexcept;

};


inline void ImGuiApplicationAssembly::set_hidden(bool hidden) noexcept {
    this->hidden_           = hidden;
    window_settings_.hidden = hidden;
    vfs_control_    .hidden = hidden;
    stage_hooks_    .hidden = hidden;
    registry_hooks_ .hidden = hidden;
}


inline void ImGuiApplicationAssembly::display() {
    vfs_control_    .display();
    window_settings_.display();
    stage_hooks_    .display();
    registry_hooks_ .display();

    context_.render();
}


} // namespace josh
