#include "Game.hpp"
#include "GlobalsGL.hpp"
#include "Input.hpp"
#include "PhysicsSystem.hpp"
#include <glm/geometric.hpp>

Game::Game(glfw::Window& window)
    : window_{ window }
    , renderer_{ glm::ortho(0.f, 1600.f, 0.f, 900.f, -1.f, 1.f) }
    , physics_{ registry_ }
    , input_{ window_ }
{
    hook_inputs();
    init_registry();
    levels_.emplace_back(GameLevel::from_file("src/breakout2/levels/one.lvl"));
    current_level().build_level_entities(registry_, physics_);
    init_walls();
}


void Game::process_input() {

    auto view = registry_.view<const InputMoveComponent, PhysicsComponent>();

    auto move_func = [&](float velocity_x_sign) {
        view.each(
            [=](const InputMoveComponent& imc, PhysicsComponent& p) {
                p.set_velocity(
                    { p.get_velocity().x + velocity_x_sign * imc.max_velocity, 0.f }
                );
            }
        );
    };

    while (!input_queue_.empty()) {
        auto event = input_queue_.front();

        switch (event) {
            using enum InputEvent;
            case lmove:
                move_func(-1.f);
                break;
            case lstop:
                move_func(1.f);
                break;
            case rmove:
                move_func(1.f);
                break;
            case rstop:
                move_func(-1.f);
                break;
            case launch_ball:
                this->launch_ball();
                break;
            case exit:
                window_.setShouldClose(true);
                break;
            default:
                break;
        }

        input_queue_.pop();
    }
}


void Game::update() {
    physics_.update(registry_, update_time_step);

    // This does not work...
    // auto& p = registry_.get<PhysicsComponent>(ball_);
    // p.set_velocity(ball_base_speed * glm::normalize(p.get_velocity()));
}


void Game::render() {
    renderer_.draw_sprites(registry_);
}



void Game::launch_ball() {
    // This is scuffed in a way, but sort of makes sense.
    if (registry_.remove<InputMoveComponent>(ball_)) {

        auto& player_p = registry_.get<const PhysicsComponent>(player_);

        registry_.patch<PhysicsComponent>(ball_, [&](PhysicsComponent& p) {
            p.set_velocity(
                ball_base_speed * glm::normalize(
                    glm::vec2{ 0.f, ball_base_speed } + player_p.get_velocity()
                )
            );
        });

    }
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
    const glm::vec2 ball_pos{ player_pos + glm::vec2{ 0.f, ball_scale.y / 2.f } };

    registry_.emplace<Transform2D>(ball_, ball_pos, ball_scale, 0.f);
    registry_.emplace<PhysicsComponent>(ball_, physics_.create_ball(ball_, ball_pos, ball_scale.x / 2.f));
    registry_.emplace<Sprite>(ball_,
        learn::globals::texture_handle_pool.load("src/breakout2/sprites/awesomeface.png"),
        ZDepth::foreground
    );
    registry_.emplace<InputMoveComponent>(ball_, player_base_speed);

    auto background = registry_.create();
    registry_.emplace<Transform2D>(background, glm::vec2{ 800.f, 450.f }, glm::vec2( 1600.f, 900.f ), 0.f);
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

    input_.set_keybind(KeyCode::A, [this](const KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            input_queue_.push(InputEvent::lmove);
        }
        if (args.is_released()) {
            input_queue_.push(InputEvent::lstop);
        }
    });

    input_.set_keybind(KeyCode::D, [this](const KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            input_queue_.push(InputEvent::rmove);
        }
        if (args.is_released()) {
            input_queue_.push(InputEvent::rstop);
        }
    });

    input_.set_keybind(KeyCode::Escape, [this](const KeyCallbackArgs& args) {
        if (args.is_released()) {
            input_queue_.push(InputEvent::exit);
        }
    });

    input_.set_keybind(KeyCode::Space, [this](const KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            input_queue_.push(InputEvent::launch_ball);
        }
    });
}
