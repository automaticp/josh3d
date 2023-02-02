#pragma once
#include "Input.hpp"
#include "SpriteRenderSystem.hpp"
#include <entt/entity/fwd.hpp>
#include <glfwpp/window.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <queue>


struct Velocity2D {
    glm::vec2 velocity;
};

struct InputMoveComponent {
    float max_velocity;
};


enum class InputEvent {
    lmove, lstop, rmove, rstop, launch_ball, exit
};



class Game {
private:
    glfw::Window& window_;
    entt::registry registry_;
    SpriteRenderSystem renderer_;
    learn::BasicRebindableInput<> input_;
    std::queue<InputEvent> input_queue_;

    entt::entity player_;
    entt::entity ball_;


    static constexpr float player_base_speed{ 1000.f };
    static constexpr float ball_base_speed{ 1000.f };

public:
    Game(glfw::Window& window);

    void process_input();
    void update();
    void render();

private:
    void hook_inputs();
    void init_registry();

    void launch_ball();
};



