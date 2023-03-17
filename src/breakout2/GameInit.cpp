#include "Game.hpp"
#include "Canvas.hpp"
#include "SpriteRenderSystem.hpp"
#include "Levels.hpp"
#include "PhysicsSystem.hpp"
#include "Transform2D.hpp"
#include "GlobalsGL.hpp"
#include <glfwpp/window.h>


Game::Game(glfw::Window& window)
    : window_{ window }
    , renderer_{
        glm::ortho(
            canvas.bound_left(), canvas.bound_right(),
            canvas.bound_bottom(), canvas.bound_top(),
            ZDepth::near, ZDepth::far
        )
    }
    , physics_{ registry_ }
    , input_{ window_ }
    , imgui_{ window_ }
{
    hook_inputs();
    init_registry();
    levels_.emplace(GameLevel::from_file("src/breakout2/levels/one.lvl"));
    levels_.current().build_level_entities(registry_, physics_);
    init_walls();
    // Ewww, sticky
    sticky_joint_ = physics_.weld(player_, ball_);
}


void Game::init_registry() {

    player_ = registry_.create();
    const glm::vec2 player_scale{ 200.f, 30.f };
    const glm::vec2 player_pos{ 800.f, 30.f };

    registry_.emplace<Transform2D>(player_, player_pos, player_scale, 0.f);
    registry_.emplace<PhysicsComponent>(player_, physics_.create_paddle(player_, player_pos, player_scale));
    registry_.emplace<Sprite>(player_,
        learn::globals::texture_handle_pool.load("src/breakout2/sprites/paddle.png"),
        ZDepth::foreground
    );
    registry_.emplace<InputMoveComponent>(player_, player_base_speed);

    ball_ = registry_.create();
    const glm::vec2 ball_scale{ 30.f, 30.f };
    const glm::vec2 ball_pos{ player_pos + glm::vec2{ 0.f, player_scale.y / 2.f + ball_scale.y / 2.f } };

    registry_.emplace<Transform2D>(ball_, ball_pos, ball_scale, 0.f);
    registry_.emplace<PhysicsComponent>(ball_, physics_.create_ball(ball_, ball_pos, ball_scale.x / 2.f));
    registry_.emplace<Sprite>(ball_,
        learn::globals::texture_handle_pool.load("src/breakout2/sprites/awesomeface.png"),
        ZDepth::foreground
    );
    // registry_.emplace<InputMoveComponent>(ball_, player_base_speed);

    auto background = registry_.create();
    registry_.emplace<Transform2D>(background, canvas.center, canvas.size, 0.f);
    registry_.emplace<Sprite>(background,
        learn::globals::texture_handle_pool.load("src/breakout2/sprites/background.jpg"),
        ZDepth::background
    );

}


void Game::init_walls() {
    constexpr float thickness{ 100.f };

    auto left = registry_.create();
    registry_.emplace<PhysicsComponent>(left,
        physics_.create_wall(left, { canvas.bound_left() - thickness / 2.f, canvas.center.y }, glm::vec2{ thickness, canvas.height() })
    );

    auto right = registry_.create();
    registry_.emplace<PhysicsComponent>(right,
        physics_.create_wall(right, { canvas.bound_right() + thickness / 2.f, canvas.center.y }, glm::vec2{ thickness, canvas.height() })
    );

    auto top = registry_.create();
    registry_.emplace<PhysicsComponent>(top,
        physics_.create_wall(top, { canvas.center.x, canvas.bound_top() + thickness / 2.f }, glm::vec2{ canvas.width(), thickness })
    );

    auto bottom = registry_.create();
    registry_.emplace<PhysicsComponent>(bottom,
        physics_.create_wall(bottom, { canvas.center.x, canvas.bound_bottom() - thickness / 2.f }, glm::vec2{ canvas.width(), thickness })
    );

}



void Game::hook_inputs() {
    using glfw::KeyCode;
    using learn::KeyCallbackArgs;
    using enum InputEvent;

    input_.set_keybind(KeyCode::A, [](const KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            events.push_input_event(lmove);
        }
        if (args.is_released()) {
            events.push_input_event(lstop);
        }
    });

    input_.set_keybind(KeyCode::D, [](const KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            events.push_input_event(rmove);
        }
        if (args.is_released()) {
            events.push_input_event(rstop);
        }
    });

    input_.set_keybind(KeyCode::Escape, [](const KeyCallbackArgs& args) {
        if (args.is_released()) {
            events.push_input_event(exit);
        }
    });

    input_.set_keybind(KeyCode::Space, [](const KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            events.push_input_event(launch_ball);
        }
    });

    // FIXME: Remove later
    input_.set_keybind(KeyCode::H, [this](const KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            fx_manager_.enable_for(FXType::speed, 15.f);
        }
    });
}

