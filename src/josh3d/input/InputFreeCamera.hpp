#pragma once
#include "PerspectiveCamera.hpp"
#include "Input.hpp"
#include "FrameTimer.hpp"
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/glm.hpp>
#include <glfwpp/glfwpp.h>
#include <glm/gtc/quaternion.hpp>


namespace josh {


struct InputFreeCameraConfig {
    using key_t = decltype(glfw::KeyCode::A);
    key_t up            { key_t::Space };
    key_t down          { key_t::LeftShift };
    key_t left          { key_t::A };
    key_t right         { key_t::D };
    key_t forward       { key_t::W };
    key_t back          { key_t::S };
    key_t toggle_cursor { key_t::C };
    // FIXME: Does this have anything to do with
    // the free camera input?
    key_t close_window  { key_t::Escape };
};


class InputFreeCamera {
private:
    PerspectiveCamera& camera_;
    InputFreeCameraConfig config_;

    struct State {
        bool up		{ false };
        bool down	{ false };
        bool left	{ false };
        bool right	{ false };
        bool forward{ false };
        bool back	{ false };
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

    InputFreeCamera(PerspectiveCamera& camera, InputFreeCameraConfig config = {})
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

    OrthonormalBasis3D cam_basis = camera_.get_local_basis();

    glm::vec3 sum_move{ 0.f };

    if (state_.up)      sum_move += cam_basis.y();
    if (state_.down)    sum_move -= cam_basis.y();
    if (state_.right)   sum_move += cam_basis.x();
    if (state_.left)    sum_move -= cam_basis.x();
    if (state_.back)    sum_move += cam_basis.z();
    if (state_.forward) sum_move -= cam_basis.z();

    if (sum_move != glm::vec3{ 0.f }) {
        camera_.transform.translate(abs_move * glm::normalize(sum_move));
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
                look_sensitivity * camera_.get_params().fovy_rad;

            const float xoffset_deg =
                sensitivity * (xpos - state_.last_xpos);

            const float yoffset_deg =
                sensitivity * (ypos - state_.last_ypos);


            if (!state_.is_cursor_mode) {

                // pitch, yaw, roll
                glm::vec3 euler = camera_.transform.get_euler();

                euler.x -= glm::radians(yoffset_deg);
                euler.y -= glm::radians(xoffset_deg);

                euler.x  = glm::clamp(euler.x,
                    glm::radians(-89.f), glm::radians(89.f));

                camera_.transform.set_euler(euler);

            }

            state_.last_xpos = xpos;
            state_.last_ypos = ypos;

        }
    );

    input.set_scroll_callback(
        [this](const ScrollCallbackArgs& args) {

            float new_fov{
                camera_.get_params().fovy_rad - zoom_sensitivity *
                glm::radians(static_cast<float>(args.yoffset))
            };

            new_fov = glm::clamp(new_fov,
                glm::radians(zoom_bounds[0]), glm::radians(zoom_bounds[1]));

            auto params = camera_.get_params();
            params.fovy_rad = new_fov;
            camera_.update_params(params);

        }
    );


    const auto to_state =
        [](const KeyCallbackArgs& args, bool& direction) {
            if (args.is_pressed() || args.is_released()) {
                direction = static_cast<bool>(args.state);
            }
        };

    input.bind_key(config_.up,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.up); });
    input.bind_key(config_.down,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.down); });
    input.bind_key(config_.left,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.left); });
    input.bind_key(config_.right,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.right); });
    input.bind_key(config_.forward,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.forward); });
    input.bind_key(config_.back,
        [&, this](const KeyCallbackArgs& args) { to_state(args, state_.back); });


    input.bind_key(config_.close_window,
        [] (const KeyCallbackArgs& args) {
            if (args.is_released()) {
                args.window.setShouldClose(true);
            }
        });

    input.bind_key(config_.toggle_cursor,
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

}




} // namespace josh
