#pragma once
#include "Runtime.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "Input.hpp"
#include "InputFreeCamera.hpp"
#include "Semantics.hpp"
#include <glfwpp/window.h>
#include <ostream>


namespace josh { class GBuffer; }

/*
HMM: Why does this exist? What purpose does it serve outside of executing
some initialization code and whatever's in the `update()`?
*/
struct DemoScene
    : josh::Immovable<DemoScene>
{
    DemoScene(
        glfw::Window&  window,
        josh::Runtime& runtime);

    void execute_frame();
    auto is_done() const noexcept -> bool;

    void process_input();
    void update();

    auto get_log_sink() -> std::ostream& { return _imgui.get_log_sink(); }

    ~DemoScene() noexcept;

    glfw::Window&           window;
    josh::Runtime&          runtime;

    josh::SimpleInputBlocker   _input_blocker;
    josh::BasicRebindableInput _input;
    josh::InputFreeCamera      _input_freecam;

    josh::ImGuiApplicationAssembly _imgui;

private:
    void configure_input();
    void init_registry();
    void update_input_blocker_from_imgui_io_state();
};

