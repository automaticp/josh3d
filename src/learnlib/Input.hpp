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
protected:
    glfw::Window& window_;

    struct KeyCallbackArgs {
        glfw::Window& window;
        glfw::KeyCode key;
        int scancode;
        glfw::KeyState state;
        glfw::ModifierKeyBit mods;
    };

    struct CursorPosCallbackArgs {
        glfw::Window& window;
        double xpos;
        double ypos;
    };

    struct ScrollCallbackArgs {
        glfw::Window& window;
        double xoffset;
        double yoffset;
    };

    // Response invoked on callback events
    virtual void respond_to_key(const KeyCallbackArgs& args) = 0;
    virtual void respond_to_cursor_pos(const CursorPosCallbackArgs& args) = 0;
    virtual void respond_to_scroll(const ScrollCallbackArgs& args) = 0;

public:
    explicit IInput(glfw::Window& window) : window_{ window } {

        window_.keyEvent.setCallback(
            [this](glfw::Window& window, glfw::KeyCode key, int scancode, glfw::KeyState state, glfw::ModifierKeyBit mods) {
                this->respond_to_key({ window, key, scancode, state, mods });
            }
        );

        window_.cursorPosEvent.setCallback(
            [this](auto&&... args){
                this->respond_to_cursor_pos({ std::forward<decltype(args)>(args)... });
            }
        );

        window_.scrollEvent.setCallback(
            [this](auto&&... args){
                this->respond_to_scroll({ std::forward<decltype(args)>(args)... });
            }
        );

    }

    // Updates referenced members (or global state) depending on the state of the input instance.
    // Must be called after each glfwPollEvents().
    virtual void process_input() = 0;

    virtual ~IInput() = default;

};





struct InputConfigFreeCamera {
    using key_t = decltype(glfw::KeyCode::A);
    key_t up            { key_t::Space };
    key_t down          { key_t::LeftShift };
    key_t left          { key_t::A };
    key_t right         { key_t::D };
    key_t forward       { key_t::W };
    key_t back          { key_t::S };
    key_t toggle_line   { key_t::H };
    key_t close_window  { key_t::Escape };
};


class InputFreeCamera : public IInput {
protected:
    Camera& camera_;

    struct MoveState {
        bool up		{ false };
        bool down	{ false };
        bool left	{ false };
        bool right	{ false };
        bool forward{ false };
        bool back	{ false };
    };

    MoveState move_state_;

    bool is_line_mode_{ false };

    float last_xpos_{ 0.0f };
    float last_ypos_{ 0.0f };

public:
    InputConfigFreeCamera config;

    InputFreeCamera(glfw::Window& window, Camera& camera, const InputConfigFreeCamera& input_config = {}) :
        IInput{ window }, camera_{ camera }, config{ input_config } {}

    void process_input() override {
        process_input_move();
    }

protected:
    void respond_to_key(const KeyCallbackArgs& args) override {
        respond_close_window(args);
        respond_toggle_line_mode(args);
        respond_camera_move(args);
    }

    void respond_to_cursor_pos(const CursorPosCallbackArgs& args) override {
        respond_camera_rotate(args);
    }

    void respond_to_scroll(const ScrollCallbackArgs& args) override {
        respond_camera_zoom(args);
    }


    void process_input_move() {
        constexpr float camera_speed{ 5.0f };
        float abs_move{ camera_speed * float(global_frame_timer.delta()) };
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


    void respond_close_window(const KeyCallbackArgs& args) {
        using namespace glfw;
        if ( args.key == KeyCode::Escape && args.state == KeyState::Release ) {
            args.window.setShouldClose(true);
        }
    }

    void respond_toggle_line_mode(const KeyCallbackArgs& args) {
        using namespace glfw;
        using namespace gl;

        if ( args.key == KeyCode::H && args.state == KeyState::Release ) {
            glPolygonMode(GL_FRONT_AND_BACK, is_line_mode_ ? GL_FILL : GL_LINE);
            is_line_mode_ ^= true;
        }
    }

    void respond_camera_move(const KeyCallbackArgs& args) {
        using namespace glfw;

        if ( args.state == KeyState::Press || args.state == KeyState::Release ) {
            bool state{ static_cast<bool>(args.state) };
            // FIXME: This scales like ... Use flatmap maybe?
            // Callbacks for actions not keys is a double-maybe.
            if ( args.key == config.up )       move_state_.up = state;
            if ( args.key == config.down )     move_state_.down = state;
            if ( args.key == config.left )     move_state_.left = state;
            if ( args.key == config.right )    move_state_.right = state;
            if ( args.key == config.forward )  move_state_.forward = state;
            if ( args.key == config.back )     move_state_.back = state;
        }
    }

    void respond_camera_rotate(const CursorPosCallbackArgs& args) {

        float xpos{ static_cast<float>(args.xpos) };
        float ypos{ static_cast<float>(args.ypos) };

        float sensitivity{ 0.1f * camera_.get_fov() };

        float xoffset{ xpos - last_xpos_ };
        xoffset = glm::radians(sensitivity * xoffset);

        float yoffset{ ypos - last_ypos_ };
        yoffset = glm::radians(sensitivity * yoffset);

        last_xpos_ = xpos;
        last_ypos_ = ypos;

        camera_.rotate(xoffset, -global_basis.y());
        camera_.rotate(yoffset, -camera_.right_uv());

    }

    void respond_camera_zoom(const ScrollCallbackArgs& args) {

        constexpr float sensitivity{ 2.0f };

        camera_.set_fov(camera_.get_fov() - sensitivity * glm::radians(static_cast<float>(args.yoffset)));
        if ( camera_.get_fov() < glm::radians(5.0f) )
            camera_.set_fov(glm::radians(5.0f));
        if ( camera_.get_fov() > glm::radians(135.0f) )
            camera_.set_fov(glm::radians(135.0f));
    }

};


} // namespace learn

