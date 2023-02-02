#pragma once
#include "GLObjects.hpp"
#include "Input.hpp"
#include "Shared.hpp"
#include "SpriteRenderer.hpp"
#include "Transform.hpp"
#include <entt/entity/fwd.hpp>
#include <glfwpp/window.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <queue>

struct Transform2D {
    glm::vec2 position;
    glm::vec2 scale;
    float angle_rad;

    learn::MTransform mtransform() const noexcept {
        return learn::Transform()
            .scale({ scale, 1.f })
            .rotate(angle_rad, { 0.f, 0.f, 1.f })
            .translate({ position, 0.f });
    }
};

struct Velocity2D {
    glm::vec2 velocity;
};

struct Sprite {
    learn::Shared<learn::TextureHandle> texture;
};

struct InputMoveComponent {
    float max_velocity;
};


enum class InputEvent {
    lmove, lstop, rmove, rstop
};


class Game {
private:
    glfw::Window& window_;
    entt::registry registry_;
    learn::SpriteRenderer renderer_;
    learn::BasicRebindableInput<> input_;
    std::queue<InputEvent> input_queue_;

public:
    Game(glfw::Window& window);

    void process_input();
    void update();
    void render();

private:
    void hook_inputs();

};



