#pragma once
#include <functional>
#include <utility>
#include <glbinding/gl/gl.h>
#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>
#include "Camera.hpp"
#include "Basis.hpp"
#include "FrameTimer.hpp"

namespace learn {



class IInput {
public:
    virtual void process_input() = 0;
    virtual ~IInput() = default;
};


class InputGlobal : public IInput {
protected:
    glfw::Window& window_;

public:
    explicit InputGlobal(glfw::Window& window) : window_{ window } {
        // using namespace std::placeholders;
        // window_.keyEvent.setCallback(std::bind(&InputGlobal::callbackKeys, this, _1, _2, _3, _4, _5));
        window_.keyEvent.setCallback(
            [this](glfw::Window& window, glfw::KeyCode key, int scancode, glfw::KeyState state, glfw::ModifierKeyBit mods) {
                this->respond_to_key({ window, key, scancode, state, mods });
            }
        );
    }

    virtual void process_input() override {}

protected:
    struct KeyCallbackArgs {
        glfw::Window& window;
        glfw::KeyCode key;
        int scancode;
        glfw::KeyState state;
        glfw::ModifierKeyBit mods;
    };

    virtual void respond_to_key(const KeyCallbackArgs& args) {
        respond_close_window(args);
        respond_enable_line_mode(args);
    }
/*
    void callbackKeys(glfw::Window& window, glfw::KeyCode key, int scancode,
        glfw::KeyState state, glfw::ModifierKeyBit mods) {
        KeyCallbackArgs args{ window, key, scancode, state, mods };
        respond_to_key(args);
    }
 */
    void respond_close_window(const KeyCallbackArgs& args) {
        using namespace glfw;
        if ( args.key == KeyCode::Escape && args.state == KeyState::Release ) {
            args.window.setShouldClose(true);
        }
    }

    void respond_enable_line_mode(const KeyCallbackArgs& args) {
        using namespace glfw;
        using namespace gl;
        static bool is_line_mode{ false }; // FIXME: Is it bad that this is shared between all instances?
        if ( args.key == KeyCode::H && args.state == KeyState::Release ) {
            glPolygonMode(GL_FRONT_AND_BACK, is_line_mode ? GL_FILL : GL_LINE);
            is_line_mode ^= true;
        }
    }

};



class InputFreeCamera : public InputGlobal {
private:
    Camera& camera_;

public:
    InputFreeCamera(glfw::Window& window, Camera& camera) : InputGlobal{ window }, camera_{ camera } {
        // using namespace std::placeholders;
        // window_.cursorPosEvent.setCallback(std::bind(&InputFreeCamera::callback_camera_rotate, this, _1, _2, _3));
        // window_.scrollEvent.setCallback(std::bind(&InputFreeCamera::callback_camera_zoom, this, _1, _2, _3));
        window_.cursorPosEvent.setCallback([this](auto&&... args){ this->callback_camera_rotate(std::forward<decltype(args)>(args)...) });
        window_.scrollEvent.setCallback([this](auto&&... args){ this->callback_camera_zoom(std::forward<decltype(args)>(args)...) });
    }


    virtual void process_input() override {
        process_input_move();
    }

protected:
    virtual void respond_to_key(const KeyCallbackArgs& args) override {
        respond_close_window(args);
        respond_enable_line_mode(args);
        respond_camera_move(args);
    }

    struct MoveState {
        bool up		{ false };
        bool down	{ false };
        bool right	{ false };
        bool left	{ false };
        bool back	{ false };
        bool forward{ false };
    };

    MoveState move_state_{};



    void process_input_move() {
        constexpr float camera_speed{ 5.0f };
        float abs_move{ camera_speed * global_frame_timer.delta() };
        glm::vec3 sum_move{ 0.0f, 0.0f, 0.0f };

        if (move_state_.up)         sum_move += camera_.up_uv();
        if (move_state_.down)       sum_move -= camera_.up_uv();
        if (move_state_.right)      sum_move += camera_.right_uv();
        if (move_state_.left)       sum_move -= camera_.right_uv();
        if (move_state_.back)       sum_move += camera_.back_uv();
        if (move_state_.forward)    sum_move -= camera_.back_uv();

        // FIXME: Comparison with 0.0f is eww
        if ( sum_move != glm::vec3{ 0.0f, 0.0f, 0.0f } ) {
            camera_.move(abs_move * glm::normalize(sum_move));
        }
    }

    void respond_camera_move(const KeyCallbackArgs& args) {
        using namespace glfw;

        if ( args.state == KeyState::Press || args.state == KeyState::Release ) {
            bool state{ static_cast<bool>(args.state) };
            // FIXME: Probably switch statement is better
            if 	    ( args.key == KeyCode::W )          move_state_.forward	= state;
            else if ( args.key == KeyCode::S )          move_state_.back    = state;
            else if ( args.key == KeyCode::A )          move_state_.left    = state;
            else if ( args.key == KeyCode::D )          move_state_.right   = state;
            else if ( args.key == KeyCode::LeftShift )  move_state_.down    = state;
            else if ( args.key == KeyCode::Space )      move_state_.up      = state;

        }
    }

    void callback_camera_rotate(glfw::Window& window, double xpos, double ypos) {

        // FIXME: no statics for god's sake
        static float last_xpos{ 0.0f };
        static float last_ypos{ 0.0f };

        float sensitivity{ 0.1f * camera_.get_fov() };

        float xoffset{ static_cast<float>(xpos) - last_xpos };
        xoffset = glm::radians(sensitivity * xoffset);

        float yoffset{ static_cast<float>(ypos) - last_ypos };
        yoffset = glm::radians(sensitivity * yoffset);

        last_xpos = static_cast<float>(xpos);
        last_ypos = static_cast<float>(ypos);

        camera_.rotate(xoffset, -global_basis.y());
        camera_.rotate(yoffset, -camera_.right_uv());

    }

    void callback_camera_zoom(glfw::Window& window, double, double yoffset) {

        constexpr float sensitivity{ 2.0f };

        camera_.set_fov(camera_.get_fov() - sensitivity * glm::radians(static_cast<float>(yoffset)));
        if ( camera_.get_fov() < glm::radians(5.0f) )
            camera_.set_fov(glm::radians(5.0f));
        if ( camera_.get_fov() > glm::radians(135.0f) )
            camera_.set_fov(glm::radians(135.0f));
    }

};


} // namespace learn

