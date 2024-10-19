#pragma once
#include "Camera.hpp"
#include "Input.hpp"
#include "Transform.hpp"
#include <entt/entity/fwd.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/glm.hpp>
#include <glfwpp/glfwpp.h>
#include <glm/gtc/quaternion.hpp>


namespace josh {


// FIXME: This is bad. I'm sorry.
class InputFreeCamera {
public:
    struct Config {
        using Key = decltype(glfw::KeyCode::A);
        Key up            { Key::R };
        Key down          { Key::F };
        Key left          { Key::A };
        Key right         { Key::D };
        Key forward       { Key::W };
        Key back          { Key::S };
        Key toggle_cursor { Key::C };
    };

    // World units per second.
    float camera_speed{ 5.0f };
    // Rotation degrees per pixel over fov.
    // rotation_deg = base_sensitivity * offset_px * fov_rad
    float look_sensitivity{ 0.1f };
    // Means something... Default is alright.
    // new_fov - old_fov = zoom_sensetivity * radians(-yoffset)
    float zoom_sensitivity{ 2.0f };
    // In degrees.
    glm::vec2 zoom_bounds{ 5.0f, 150.0f };

    InputFreeCamera() = default;
    InputFreeCamera(Config config) : config_{ config } {}

    const auto& state() const noexcept { return state_; }

    // Call every frame.
    void update(float dt, Camera& camera, Transform& cam_tf);

    // Setup input with the current configuration.
    // Public parameters of InputFreeCamera can be changed
    // at runtime without a need to reconfigure.
    void configure(BasicRebindableInput& input);

private:
    Config config_{};

    struct State {
        bool up		{ false };
        bool down	{ false };
        bool left	{ false };
        bool right	{ false };
        bool forward{ false };
        bool back	{ false };
        bool is_cursor_mode{ false };
        float last_xpos    { 0.0f };
        float last_ypos    { 0.0f };
        float delta_xpos   { 0.0f };
        float delta_ypos   { 0.0f };
        float delta_yscroll{ 0.0f };
    } state_;
};


} // namespace josh
