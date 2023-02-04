#pragma once
#include "Input.hpp"
#include "SpriteRenderSystem.hpp"
#include "PhysicsSystem.hpp"
#include "GameLevel.hpp"
#include <box2d/b2_joint.h>
#include <entt/entity/fwd.hpp>
#include <glfwpp/window.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <queue>
#include <vector>



struct InputMoveComponent {
    float max_velocity;
    bool wants_move_left{ false };
    bool wants_move_right{ false };
};



class Game {
private:
    glfw::Window& window_;
    entt::registry registry_;
    SpriteRenderSystem renderer_;
    PhysicsSystem physics_;
    learn::BasicRebindableInput<> input_;

    std::vector<GameLevel> levels_;
    size_t current_level_{ 0 };

    entt::entity player_;
    entt::entity ball_;

    b2Joint* sticky_joint_;

    static constexpr float player_base_speed{ 1000.f };
    static constexpr float ball_base_speed{ 700.f };

public:
    // Fixed time step chosen for the game logic update rate.
    static constexpr float update_time_step{ 1.f / 240.f };

    Game(glfw::Window& window);

    void process_events();
    void update();
    void render();

private:
    void process_input_events();
    void process_tile_collision_events();

    void update_transforms();
    void update_player_velocity();

    void hook_inputs();
    void init_registry();
    void init_walls();

    GameLevel& current_level() noexcept { return levels_[current_level_]; }

    void launch_ball();
};



