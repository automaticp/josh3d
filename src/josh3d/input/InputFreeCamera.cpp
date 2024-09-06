#include "InputFreeCamera.hpp"
#include <glm/glm.hpp>


namespace josh {


void InputFreeCamera::update(float dt, Camera& camera, Transform& cam_tf) {
    using glm::mat4, glm::mat3, glm::vec3;

    // Camera movement.
    {
        const vec3 move_direction = [this]{
            constexpr mat3 basis{ 1.f };
            vec3 direction{ 0.f };
            if (state_.up)      direction += basis[1];
            if (state_.down)    direction -= basis[1];
            if (state_.right)   direction += basis[0];
            if (state_.left)    direction -= basis[0];
            if (state_.back)    direction += basis[2];
            if (state_.forward) direction -= basis[2];

            if (direction != vec3(0.f)) {
                return normalize(direction); // Normalizing null-vectors is UB. Great!
            } else {
                return direction;
            }
        }();
        const float move_magnitude = camera_speed * dt;

        const mat4 T_camera  = glm::translate(move_magnitude * move_direction); // Translation matrix in camera-space.
        const mat4 P2C       = cam_tf.mtransform().model(); // Parent->Camera CoB.
        const mat4 C2P       = inverse(P2C);
        const mat4 T_parent  = P2C * T_camera * C2P;
        const vec3 dr_parent = T_parent[3];

        cam_tf.translate(dr_parent);
    }


    // Camera rotation.
    {
        const float sensitivity = look_sensitivity * camera.get_params().fovy_rad;
        const float xoffset_deg = sensitivity * state_.delta_xpos;
        const float yoffset_deg = sensitivity * state_.delta_ypos;

        if (!state_.is_cursor_mode) {
            vec3 euler = cam_tf.get_euler(); // Pitch, Yaw, Roll.
            euler.x -= glm::radians(yoffset_deg);
            euler.y -= glm::radians(xoffset_deg);
            euler.x  = glm::clamp(euler.x, glm::radians(-89.f), glm::radians(89.f));
            cam_tf.set_euler(euler);
        }

        // Reset deltas so they are not applied next frame if there was no input.
        state_.delta_xpos = 0.f;
        state_.delta_ypos = 0.f;
    }

    // Camera zoom.
    {
        float new_fov = camera.get_params().fovy_rad - zoom_sensitivity * glm::radians(state_.delta_yscroll);
        new_fov = glm::clamp(new_fov, glm::radians(zoom_bounds[0]), glm::radians(zoom_bounds[1]));

        auto params = camera.get_params();
        params.fovy_rad = new_fov;
        camera.update_params(params);

        state_.delta_yscroll = 0.f;
    }

}




void InputFreeCamera::configure(BasicRebindableInput& input) {

    state_.is_cursor_mode =
        input.window().getInputModeCursor() == glfw::CursorMode::Normal;

    input.set_cursor_pos_callback(
        [this](const CursorPosCallbackArgs& args) {

            const auto xpos = float(args.xpos);
            const auto ypos = float(args.ypos);

            // Accumulate delta. Use last_*pos for computing intermediate deltas.
            state_.delta_xpos += (xpos - state_.last_xpos);
            state_.delta_ypos += (ypos - state_.last_ypos);

            state_.last_xpos = xpos;
            state_.last_ypos = ypos;

        }
    );

    input.set_scroll_callback(
        [this](const ScrollCallbackArgs& args) {

            state_.delta_yscroll += float(args.yoffset);

        }
    );


    const auto to_state =
        [](const KeyCallbackArgs& args, bool& direction) {
            if (args.is_pressed() || args.is_released()) {
                direction = static_cast<bool>(args.state);
            }
        };

    input.bind_key(config_.up,      [&, this](const KeyCallbackArgs& args) { to_state(args, state_.up);      });
    input.bind_key(config_.down,    [&, this](const KeyCallbackArgs& args) { to_state(args, state_.down);    });
    input.bind_key(config_.left,    [&, this](const KeyCallbackArgs& args) { to_state(args, state_.left);    });
    input.bind_key(config_.right,   [&, this](const KeyCallbackArgs& args) { to_state(args, state_.right);   });
    input.bind_key(config_.forward, [&, this](const KeyCallbackArgs& args) { to_state(args, state_.forward); });
    input.bind_key(config_.back,    [&, this](const KeyCallbackArgs& args) { to_state(args, state_.back);    });

    input.bind_key(config_.toggle_cursor,
        [&, this] (const KeyCallbackArgs& args) {
            if (args.is_released()) {
                state_.is_cursor_mode ^= true;
                args.window.setInputModeCursor(
                    state_.is_cursor_mode ? glfw::CursorMode::Normal : glfw::CursorMode::Disabled
                );
            }
        });

}



} // namespace josh
