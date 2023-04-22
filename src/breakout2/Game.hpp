#pragma once
#include "FXStateManager.hpp"
#include "Input.hpp"
#include "PowerUpGenerator.hpp"
#include "GameRenderSystem.hpp"
#include "PhysicsSystem.hpp"
#include "Levels.hpp"
#include "ImGuiContextWrapper.hpp"
#include "ImGuiInputBlocker.hpp"
#include <box2d/b2_joint.h>
#include <entt/entity/fwd.hpp>
#include <glfwpp/window.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <queue>
#include <unordered_set>
#include <vector>



struct InputMoveComponent {
    float max_velocity;
    bool wants_move_left{ false };
    bool wants_move_right{ false };
};

/*
It's like a Trash Bin for entities that's cleared on every update!

Prefer the Trash Bin over destroying entities inplace, as other
systems could still depend on the entity in the current frame.
For example, the physics system could register multiple collisions
in the same update and try to destroy the entity on every collision,
resulting in double- and sometimes triple-free scenarios.

I'm bringing this up as an example not because it's such an obvious
observation and I definetly would never have this happen to me...
*/
class EntityTrashBin {
private:
    entt::registry& registry_;
    std::unordered_set<entt::entity> entities_to_destroy_;

public:
    EntityTrashBin(entt::registry& registry) : registry_{ registry } {}

    void destroy_later(entt::entity entity) {
        entities_to_destroy_.emplace(entity);
    }

    void clear_out() {
        for (auto ent : entities_to_destroy_) {
            registry_.destroy(ent);
        }
        entities_to_destroy_.clear();
    }
};



class Game {
private:
    glfw::Window& window_;
    entt::registry registry_;
    GameRenderSystem renderer_;
    PhysicsSystem physics_;

    PowerUpGenerator powerup_generator_;

    learn::ImGuiInputBlocker input_blocker_;
    learn::BasicRebindableInput input_;

    learn::ImGuiContextWrapper imgui_;

    FXStateManager fx_manager_;

    Levels levels_;

    entt::entity player_;
    entt::entity ball_;

    EntityTrashBin trash_;

    b2Joint* sticky_joint_;

    static constexpr float player_base_speed{ 1000.f };
    static constexpr float ball_base_speed{ 700.f };

public:
    // Fixed time step chosen for the game logic update rate.
    static constexpr float update_time_step{ 1.f / 240.f };

    Game(glfw::Window& window);

    // TODO: Maybe process some events before update
    // and some after?
    void process_events();
    void update();
    void render();

private:
    void process_input_events();
    void process_tile_collision_events();
    void process_powerup_collision_events();
    void process_fx_state_updates();

    void launch_ball();

    void update_transforms();
    void update_player_velocity();
    void update_fx_state();

    void update_gui();

    void hook_inputs();
    void init_registry();
    void init_walls();

};



