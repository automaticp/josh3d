#pragma once
#include <functional>
#include <utility>
#include <unordered_map>
#include <concepts>
#include <glbinding/gl/gl.h>
#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>
#include "Camera.hpp"
#include "Basis.hpp"
#include "FrameTimer.hpp"

namespace learn {


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






class BasicRebindableInput {
public:
    using key_t = decltype(glfw::KeyCode::A);
    using keymap_t = std::unordered_map<key_t, std::function<void(const KeyCallbackArgs&)>>;

private:
    keymap_t keymap_;

protected:
    glfw::Window& window_;


public:
    explicit BasicRebindableInput(glfw::Window& window) : window_{ window } {}

    BasicRebindableInput(glfw::Window& window, keymap_t keymap)
        : window_{ window }, keymap_{ std::move(keymap) } {}


    void set_keybind(key_t key, std::function<void(const KeyCallbackArgs&)> callback) {
        keymap_.insert_or_assign(key, std::move(callback));
    }


    template<std::invocable CallbackT>
    void set_cursor_pos_callback(CallbackT&& callback) {

        set_glfw_callback<CursorPosCallbackArgs>(
            window_.cursorPosEvent, std::forward<CallbackT>(callback)
        );

    }

    template<std::invocable CallbackT>
    void set_scroll_callback(CallbackT&& callback) {

        set_glfw_callback<ScrollCallbackArgs>(
            window_.scrollEvent, std::forward<CallbackT>(callback)
        );

    }

    // Updates referenced members (or global state)
    // depending on the state of the input instance.
    // Must be called after each glfwPollEvents().
    virtual void process_input() {}

    virtual ~BasicRebindableInput() = default;

private:
    // We use a templated setter to basically 'emplace' arbitrary callable
    // into the glfwpp callback (implemented as std::function) for corresponding event.
    //
    // However, we also want to pack the arguments in one of the 'CallbackArgs' structs,
    // and force the user-specified callback to the signature compatible with:
    //     void(const CallbackArgs&)
    //
    // So we first pack arguments via a proxy 'respond_to' function,
    // and then inside that function actually invoke the callback.
    //
    template<typename CallbackArgsT, std::invocable CallbackT, typename ...EventArgs>
    void set_glfw_callback(glfw::Event<EventArgs...>& event, CallbackT&& callback) {

        event.setCallback(
            [this, &callback]<typename ...Args>(Args&&... args) {
                respond_to<CallbackArgsT>(
                    std::forward<CallbackT>(callback), { std::forward<Args>(args)... }
                );
            }
        );

    }

    template<typename CallbackArgsT, std::invocable CallbackT>
    void respond_to(CallbackT&& callback, const CallbackArgsT& args) {
        callback(args);
    }


    // For keys we set our own special callback, that
    // indexes into a keymap with KeyCallbackArgs::key
    // and calls the corresponding used-defined callback,
    // if it exists.
    void enable_key_callback() {
        window_.keyEvent.setCallback(
            [this]<typename ...Args>(Args&&... args) {
                respond_to_key({ std::forward<Args>(args)... });
            }
        );
    }

    void respond_to_key(const KeyCallbackArgs& args) {
        // Maybe better to just use [] to avoid branching?
        auto it = keymap_.find(args.key);
        if ( it != keymap_.end() ) {
            std::invoke(it->second, args);
        }
    }

};





class IInput {
protected:
    glfw::Window& window_;

    // Response invoked on callback events
    virtual void respond_to_key(const KeyCallbackArgs& args) = 0;
    virtual void respond_to_cursor_pos(const CursorPosCallbackArgs& args) = 0;
    virtual void respond_to_scroll(const ScrollCallbackArgs& args) = 0;

public:
    explicit IInput(glfw::Window& window) : window_{ window } {}

    // Call this to enable or switch input depending on concrete Input class
    void bind_callbacks() {

        window_.keyEvent.setCallback(
            [this](auto&&... args) {
                this->respond_to_key({ std::forward<decltype(args)>(args)... });
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

    // Enables this input. The same as bind_callbacks().
    void use() { bind_callbacks(); }

    // Updates referenced members (or global state) depending on the state of the input instance.
    // Must be called after each glfwPollEvents().
    virtual void process_input() = 0;

    virtual ~IInput() = default;

};



class RebindableInput : public virtual IInput {
public:
    using key_t = decltype(glfw::KeyCode::A);
    using map_t = std::unordered_map<key_t, std::function<void(const KeyCallbackArgs&)>>;

private:
    map_t keymap_;

public:
    explicit RebindableInput(glfw::Window& window) : IInput(window) {}

    RebindableInput& set_keybind(key_t key, std::function<void(const KeyCallbackArgs&)> callback) {
        keymap_.emplace(key, std::move(callback));
        return *this;
    }

protected:

    void respond_to_key(const KeyCallbackArgs& args) override {
        // Maybe better to just use [] to avoid branching?
        auto it = keymap_.find(args.key);
        if ( it != keymap_.end() ) {
            std::invoke(it->second, args);
        }
    }

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
    key_t toggle_cursor { key_t::C };
    key_t close_window  { key_t::Escape };
};


class InputFreeCamera : public virtual IInput {
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
    bool is_cursor_mode_{ false };

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
        respond_toggle_cursor(args);
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
        if ( args.key == config.close_window && args.state == KeyState::Release ) {
            args.window.setShouldClose(true);
        }
    }

    void respond_toggle_line_mode(const KeyCallbackArgs& args) {
        using namespace glfw;
        using namespace gl;

        if ( args.key == config.toggle_line && args.state == KeyState::Release ) {
            is_line_mode_ ^= true;
            glPolygonMode(GL_FRONT_AND_BACK, is_line_mode_ ? GL_LINE : GL_FILL);
        }
    }

    void respond_toggle_cursor(const KeyCallbackArgs& args) {
        using namespace glfw;

        if ( args.key == config.toggle_cursor && args.state == KeyState::Release ) {
            is_cursor_mode_ ^= true;
            args.window.setInputModeCursor( is_cursor_mode_ ? CursorMode::Normal : CursorMode::Disabled );
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

        if ( !is_cursor_mode_ ) {
            camera_.rotate(xoffset, -global_basis.y());
            camera_.rotate(yoffset, -camera_.right_uv());
        }
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


// How to commit a sin 101
class RebindableInputFreeCamera : public InputFreeCamera, public RebindableInput {
protected:
    void respond_to_key(const KeyCallbackArgs& args) override {
        InputFreeCamera::respond_to_key(args);
        RebindableInput::respond_to_key(args);
    }

public:
    RebindableInputFreeCamera(glfw::Window& window, Camera& camera, const InputConfigFreeCamera& input_config = {}) :
        InputFreeCamera(window, camera, input_config), RebindableInput(window), IInput(window) {}

};





} // namespace learn

