#pragma once
#include <glfwpp/glfwpp.h>
#include <glfwpp/window.h>
#include <concepts>
#include <functional>
#include <unordered_map>
#include <utility>


namespace josh {


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

    bool is_pressed() const noexcept { return state == glfw::KeyState::Press; }
    bool is_released() const noexcept { return state == glfw::KeyState::Release; }
    bool is_repeated() const noexcept { return state == glfw::KeyState::Repeat; }
};

struct MouseButtonCallbackArgs {
    glfw::Window& window;
    glfw::MouseButton button;
    glfw::MouseButtonState state;
    glfw::ModifierKeyBit mods;

    bool is_pressed() const noexcept { return state == glfw::MouseButtonState::Press; }
    bool is_released() const noexcept { return state == glfw::MouseButtonState::Release; }
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





/*
This little 'blocker' incident is a direct consequence
of me trying to integrate dear-imgui into the input stack.

You can implement imgui blocker by wrapping ImGui::GetIO().WantCapture* values,
or you can manually update the blocking state with SimpleInputBlocker.

A more fine-grained blocking that depends on the exact keys pressed / cursor updates
can also be implemented, although the practical usefullness of that is so far unconfirmed.
*/
class IInputBlocker {
public:
    virtual bool is_key_blocked(const KeyCallbackArgs&) const = 0;
    virtual bool is_mouse_button_blocked(const MouseButtonCallbackArgs&) const = 0;
    virtual bool is_cursor_blocked(const CursorPosCallbackArgs&) const = 0;
    virtual bool is_scroll_blocked(const ScrollCallbackArgs&) const = 0;
    virtual ~IInputBlocker() = default;
};

class NonBlockingInputBlocker final : public IInputBlocker {
public:
    bool is_key_blocked(const KeyCallbackArgs&) const override { return false; }
    bool is_mouse_button_blocked(const MouseButtonCallbackArgs&) const override { return false; }
    bool is_cursor_blocked(const CursorPosCallbackArgs&) const override { return false; };
    bool is_scroll_blocked(const ScrollCallbackArgs&) const override { return false; };
};

class SimpleInputBlocker final : public IInputBlocker {
public:
    bool block_keys         { false };
    bool block_mouse_buttons{ false };
    bool block_cursor       { false };
    bool block_scroll       { false };

    bool is_key_blocked(const KeyCallbackArgs&) const override { return block_keys; }
    bool is_mouse_button_blocked(const MouseButtonCallbackArgs&) const override { return block_mouse_buttons; }
    bool is_cursor_blocked(const CursorPosCallbackArgs&) const override { return block_cursor; };
    bool is_scroll_blocked(const ScrollCallbackArgs&) const override { return block_scroll; };
};



/*
Simple input class with a map: key -> function.
Limited in a sense that multi-key inputs are not reasonable
to implement. But works okay for testing and demos.
*/
class BasicRebindableInput {
public:
    using key_t     = decltype(glfw::KeyCode::A);
    using mbutton_t = decltype(glfw::MouseButton::Left);
    using keymap_t      = std::unordered_map<key_t, std::function<void(const KeyCallbackArgs&)>>;
    using mbutton_map_t = std::unordered_map<mbutton_t, std::function<void(const MouseButtonCallbackArgs&)>>;

private:
    glfw::Window& window_;

    // Non-owning.
    IInputBlocker* blocker_;

    keymap_t keymap_;
    mbutton_map_t mbutton_map_;


public:
    explicit BasicRebindableInput(glfw::Window& window)
        : window_{ window }
        , blocker_{
            []() -> IInputBlocker*  {
                static NonBlockingInputBlocker non_blocker;
                return &non_blocker;
            }()
        }
    {
        enable_key_callback();
        enable_mouse_button_callback();
    }

    BasicRebindableInput(glfw::Window& window, IInputBlocker& input_blocker)
        : window_{ window }
        , blocker_{ &input_blocker }
    {
        enable_key_callback();
        enable_mouse_button_callback();
    }


    glfw::Window& window() noexcept { return window_; }
    const glfw::Window& window() const noexcept { return window_; }

    void bind_key(key_t key,
        std::function<void(const KeyCallbackArgs&)> callback)
    {
        keymap_.insert_or_assign(key, std::move(callback));
    }

    void bind_mouse_button(mbutton_t mouse_button,
        std::function<void(const MouseButtonCallbackArgs&)> callback)
    {
        mbutton_map_.insert_or_assign(mouse_button, std::move(callback));
    }

    void enable_key_callback() {
        window_.keyEvent.setCallback(
            [this](auto&&... args) {
                invoke_on_key({ std::forward<decltype(args)>(args)... });
            }
        );
    }

    void enable_mouse_button_callback() {
        window_.mouseButtonEvent.setCallback(
            [this](auto&&... args) {
                invoke_on_mouse_button({ std::forward<decltype(args)>(args)... });
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
            [this, callback=std::forward<CallbackT>(callback)](auto&&... args) mutable {

                invoke_on_cursor_pos(
                    callback,
                    { std::forward<decltype(args)>(args)... }
                );

            }
        );
    }

    template<std::invocable<const ScrollCallbackArgs&> CallbackT>
    void set_scroll_callback(CallbackT&& callback) {

        window_.scrollEvent.setCallback(
            [this, callback=std::forward<CallbackT>(callback)](auto&&... args) mutable {

                invoke_on_scroll(
                    callback,
                    { std::forward<decltype(args)>(args)... }
                );

            }
        );
    }

    void reset_keymap(keymap_t new_key_map)
        noexcept(noexcept(keymap_ = std::move(new_key_map)))
    {
        keymap_ = std::move(new_key_map);
    }

    void reset_mouse_button_map(mbutton_map_t new_map)
        noexcept(noexcept(mbutton_map_ = std::move(new_map)))
    {
        mbutton_map_ = std::move(new_map);
    }


private:
    template<typename CallbackT>
    void invoke_on_cursor_pos(CallbackT& callback, const CursorPosCallbackArgs& args) {
        if (!blocker_->is_cursor_blocked(args)) {
            callback(args);
        }
    }

    template<typename CallbackT>
    void invoke_on_scroll(CallbackT& callback, const ScrollCallbackArgs& args) {
        if (!blocker_->is_scroll_blocked(args)) {
            callback(args);
        }
    }

    void invoke_on_key(const KeyCallbackArgs& args) {
        if (!blocker_->is_key_blocked(args)) {
            auto it = keymap_.find(args.key);
            if (it != keymap_.end()) {
                std::invoke(it->second, args);
            }
        }
    }

    void invoke_on_mouse_button(const MouseButtonCallbackArgs& args) {
        if (!blocker_->is_mouse_button_blocked(args)) {
            auto it = mbutton_map_.find(args.button);
            if (it != mbutton_map_.end()) {
                std::invoke(it->second, args);
            }
        }
    }

};


} // namespace josh

