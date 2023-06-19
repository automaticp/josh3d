#pragma once
#include "Camera.hpp"
#include "GlobalsUtil.hpp"
#include "Input.hpp"
#include <glm/glm.hpp>
#include <glfwpp/glfwpp.h>


namespace learn {


struct InputFreeCameraConfig {
    using key_t = decltype(glfw::KeyCode::A);
    key_t up            { key_t::Space };
    key_t down          { key_t::LeftShift };
    key_t left          { key_t::A };
    key_t right         { key_t::D };
    key_t forward       { key_t::W };
    key_t back          { key_t::S };
    key_t toggle_cursor { key_t::C };
    // FIXME: Do these have anything to do with
    // the free camera input?
    key_t toggle_line   { key_t::H };
    key_t close_window  { key_t::Escape };
};


class InputFreeCamera {
private:
    Camera& camera_;
    InputFreeCameraConfig config_;

    struct State {
        bool up		{ false };
        bool down	{ false };
        bool left	{ false };
        bool right	{ false };
        bool forward{ false };
        bool back	{ false };
        bool is_line_mode{ false };
        bool is_cursor_mode{ false };
        float last_xpos{ 0.0f };
        float last_ypos{ 0.0f };
    };

    State state_;


public:
    // World units per second.
    float camera_speed{ 5.0f };
    // Rotation degrees per pixel over fov.
    // rotation_deg = base_sensitivity * offset_px * fov_rad
    float look_sensitivity{ 0.1f };
    // Means something... Default is alright.
    // new_fov - old_fov = zoom_sensetivity * radians(-yoffset)
    float zoom_sensitivity{ 2.0f };
    // In degrees.
    glm::vec2 zoom_bounds{ 5.0f, 135.0f };

    InputFreeCamera(Camera& camera, InputFreeCameraConfig config = {})
        : camera_{ camera }
        , config_{ config }
    {}

    const State& state() const noexcept { return state_; }

    // Call every frame.
    void update();

    // Setup input with the current configuration.
    // Public parameters of InputFreeCamera can be changed
    // at runtime without a need to reconfigure.
    void configure(BasicRebindableInput& input);


};




inline void InputFreeCamera::update() {

    const float abs_move =
        camera_speed * globals::frame_timer.delta<float>();

    glm::vec3 sum_move{ 0.f };

    if (state_.up)      sum_move += camera_.up_uv();
    if (state_.down)    sum_move -= camera_.up_uv();
    if (state_.right)   sum_move += camera_.right_uv();
    if (state_.left)    sum_move -= camera_.right_uv();
    if (state_.back)    sum_move += camera_.back_uv();
    if (state_.forward) sum_move -= camera_.back_uv();

    if (sum_move != glm::vec3{ 0.f }) {
        camera_.move(abs_move * glm::normalize(sum_move));
    }

}




inline void InputFreeCamera::configure(BasicRebindableInput& input) {

    state_.is_cursor_mode =
        input.window().getInputModeCursor() == glfw::CursorMode::Normal;

    input.set_cursor_pos_callback(
        [this](const CursorPosCallbackArgs& args) {

            const auto xpos = static_cast<float>(args.xpos);
            const auto ypos = static_cast<float>(args.ypos);

            const float sensitivity =
                look_sensitivity * camera_.get_fov();

            const float xoffset_deg =
                sensitivity * (xpos - state_.last_xpos);

            const float yoffset_deg =
                sensitivity * (ypos - state_.last_ypos);

            state_.last_xpos = xpos;
            state_.last_ypos = ypos;

            if (!state_.is_cursor_mode) {
                camera_.rotate(
                    glm::radians(xoffset_deg), -globals::basis.y()
                );

                camera_.rotate(
                    glm::radians(yoffset_deg), -camera_.right_uv()
                );
            }

        }
    );

    input.set_scroll_callback(
        [this](const ScrollCallbackArgs& args) {

            float new_fov{
                camera_.get_fov() - zoom_sensitivity *
                glm::radians(static_cast<float>(args.yoffset))
            };

            camera_.set_fov(
                glm::clamp(
                    new_fov,
                    glm::radians(zoom_bounds[0]),
                    glm::radians(zoom_bounds[1])
                )
            );

        }
    );


    const auto to_state =
        [](const KeyCallbackArgs& args, bool& direction) {
            if (args.is_pressed() || args.is_released()) {
                direction = static_cast<bool>(args.state);
            }
        };

    input.set_keybind(config_.up,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.up); });
    input.set_keybind(config_.down,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.down); });
    input.set_keybind(config_.left,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.left); });
    input.set_keybind(config_.right,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.right); });
    input.set_keybind(config_.forward,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.forward); });
    input.set_keybind(config_.back,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.back); });


    input.set_keybind(config_.close_window,
        [] (const KeyCallbackArgs& args) {
            if (args.is_released()) {
                args.window.setShouldClose(true);
            }
        });

    input.set_keybind(config_.toggle_cursor,
        [&, this] (const KeyCallbackArgs& args) {
            if (args.is_released()) {
                state_.is_cursor_mode ^= true;
                args.window.setInputModeCursor(
                    state_.is_cursor_mode ?
                    glfw::CursorMode::Normal :
                    glfw::CursorMode::Disabled
                );
            }
        });

    input.set_keybind(config_.toggle_line,
        [&, this] (const KeyCallbackArgs& args) {
            using namespace gl;
            if (args.is_released()) {
                state_.is_line_mode ^= true;
                glPolygonMode(
                    GL_FRONT_AND_BACK,
                    state_.is_line_mode ?
                    GL_LINE : GL_FILL
                );
            }
        });

}




} // namespace learn
