#pragma once
#include "Basis.hpp"
#include "Camera.hpp"
#include "Globals.hpp"
#include "glfwpp/window.h"

#include <glbinding/gl/gl.h>
#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>
#include <concepts>
#include <functional>
#include <unordered_map>
#include <utility>

namespace learn {


/*
Ideally, any input system would be disconnected from the application
logic, and instead, communicate by sending events.

This implies that the key/axis events recieved from glfw have to
be translated into other events that the end application understands.
This translation layer is exactly what makes an input system.

However, no tranformation can be fully abstracted, it's the application
developer's responsibility to fill out the exact rules of this translation.

This is what creates the binding.

For an example, let's take a simple movement input.
We want to support different input devices

At the glfw event layer we have (simplified):

struct KeyEvent {
    KeyCode code;
};

struct JoyXYEvent {
    float position_x;
    float position_y;
};

Assume that we want from our application POV for these two input
events to produce identical behavior:

1. KeyEvent(KeyCode::W) && KeyEvent(KeyCode::D)
2. JoyXYEvent{ sqrt(2.f), sqrt(2.f) }

That is, us holding W and D at the same time should be
equivalent to tilting the joystick north-east.

Our application will process move events, abstracted away from the input methods

struct MoveEvent {
    float dx;
    float dy;
};

The translation layer is responsible exactly for this

KeyEvent(W) && KeyEvent(D)
    ==> KeyInputTranslation
        ==> MoveEvent{ sqrt(2.f), sqrt(2.f) }
            ==> ApplicationEventQueue

JoyXYEvent{ sqrt(2.f), sqrt(2.f) }
    ==> JoyInputTranslation
        ==> MoveEvent{ sqrt(2.f), sqrt(2.f) }
            ==> ApplicationEventQueue


But the exact rules of the translation are unknown to the InputTranslation classes.

We have at least 2 requirements for the design of the InputTranslation:
- The input events should be rebindable at runtime for different devices;
- The translation rules must be definable by a client application at compile time.

Note that the input events include their combinations, which serves as a source of
additional complexity.

Also note that the input events are rebindable to a fixed set of translation rules.
Again, think of alternative keybindings and a single action performed.

Also also note that there does not have to exist a single translation class,
instead it would be much cleaner to have a translation class for each
input device type. Again, all the device details are abstracted away
because the application receives just a MoveEvent.

If you really wish, you can even separate translation of
controller buttons from axes, although think about it maybe...


Anyways, this is supposed to be a recipe for a decent input system.

You'll find none of it below, though.

Below is only a primitive 'key -> callback' implementation, which treats
input events as application events, so no abstraction. Sad.


All written above is directed at the future me, that might one day try to actually do it.

Input is deceivingly hard...

*/



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






// This little 'blocker' incident is a direct consequence
// of me trying to integrate dear-imgui into the input stack.
//
// I also do this thing where I define a whole static interface
// with concepts and consume the blocker class as a template parameter.
// I really don't want to mix the imgui code with glfw code as much as possible.
//
// Future me may forgive me for overcomplicating things this much.
//
// Maybe just use a simple virtual interface class instead, jees...


template<typename BlockerT>
concept input_key_blocker = requires(const BlockerT& blocker, const KeyCallbackArgs& key_args) {
    { blocker.is_key_blocked(key_args) } -> std::same_as<bool>;
};

template<typename BlockerT>
concept input_cursor_blocker = requires(const BlockerT& blocker, const CursorPosCallbackArgs& key_args) {
    { blocker.is_cursor_blocked(key_args) } -> std::same_as<bool>;
};

template<typename BlockerT>
concept input_scroll_blocker = requires(const BlockerT& blocker, const ScrollCallbackArgs& key_args) {
    { blocker.is_scroll_blocked(key_args) } -> std::same_as<bool>;
};

template<typename BlockerT>
concept input_kbm_blocker =
    input_key_blocker<BlockerT> &&
    input_cursor_blocker<BlockerT> &&
    input_scroll_blocker<BlockerT>;


// Input blocker that does not block, duh.
class NonBlockingInputBlocker {
public:
    // Compiler-sama, you're smart sometimes, optimize it out pls
    constexpr bool is_key_blocked(const KeyCallbackArgs&) const noexcept { return false; }
    constexpr bool is_cursor_blocked(const CursorPosCallbackArgs&) const noexcept { return false; }
    constexpr bool is_scroll_blocked(const ScrollCallbackArgs&) const noexcept { return false; }
};




// Simple input class with a map: key -> function.
// Limited in a sense that multi-key inputs are not reasonable
// to implement. But works okay for testing and demos.
template<input_kbm_blocker BlockerT = NonBlockingInputBlocker>
class BasicRebindableInput {
public:
    using key_t = decltype(glfw::KeyCode::A);
    using keymap_t = std::unordered_map<key_t, std::function<void(const KeyCallbackArgs&)>>;

private:
    glfw::Window& window_;

    // One of the many fewerish ideas I had, sorry again
    BlockerT blocker_;

    keymap_t keymap_;

public:
    explicit BasicRebindableInput(glfw::Window& window) : window_{ window } {}

    BasicRebindableInput(glfw::Window& window, BlockerT input_blocker)
        : window_{ window }
        , blocker_{ std::move(input_blocker) }
    {}


    void set_keybind(key_t key, std::function<void(const KeyCallbackArgs&)> callback) {
        keymap_.insert_or_assign(key, std::move(callback));
    }

    void enable_key_callback() {
        window_.keyEvent.setCallback(
            [this](auto&&... args) {
                invoke_on_key({ std::forward<decltype(args)>(args)... });
            }
        );
    }


    // Ok, this is dense.
    //
    // We set the glfw callback to an internal lambda
    // that forwards the callback to the 'invoke_on_*' function
    // AND packs the arguments into a struct at the same time.
    // The 'invoke_on_*' function actually invokes the callback.
    //
    // The callback is captured by value in the lambda due to
    // potential lifetime concerns.
    template<std::invocable<const CursorPosCallbackArgs&> CallbackT>
    void set_cursor_pos_callback(CallbackT&& callback) {

        window_.cursorPosEvent.setCallback(
            [this, callback=std::forward<CallbackT>(callback)](auto&&... args) {

                invoke_on_cursor_pos(
                    std::forward<CallbackT>(callback),
                    { std::forward<decltype(args)>(args)... }
                );

            }
        );
    }

    template<std::invocable<const ScrollCallbackArgs&> CallbackT>
    void set_scroll_callback(CallbackT&& callback) {

        window_.scrollEvent.setCallback(
            [this, callback=std::forward<CallbackT>(callback)](auto&&... args) {

                invoke_on_scroll(
                    std::forward<CallbackT>(callback),
                    { std::forward<decltype(args)>(args)... }
                );

            }
        );
    }



    void reset_keymap(const keymap_t& new_keymap) noexcept(noexcept(keymap_ = new_keymap)) {
        keymap_ = new_keymap;
    }

    void reset_keymap(keymap_t&& new_keymap) noexcept(noexcept(keymap_ = std::move(new_keymap))) {
        keymap_ = std::move(new_keymap);
    }



private:
    template<typename CallbackT>
    void invoke_on_cursor_pos(CallbackT& callback, const CursorPosCallbackArgs& args) {
        if (!blocker_.is_cursor_blocked(args)) {
            callback(args);
        }
    }

    template<typename CallbackT>
    void invoke_on_scroll(CallbackT& callback, const ScrollCallbackArgs& args) {
        if (!blocker_.is_scroll_blocked(args)) {
            callback(args);
        }
    }

    void invoke_on_key(const KeyCallbackArgs& args) {
        if (!blocker_.is_key_blocked(args)) {
            auto it = keymap_.find(args.key);
            if (it != keymap_.end()) {
                std::invoke(it->second, args);
            }
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
        float abs_move{ camera_speed * float(globals::frame_timer.delta()) };
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

