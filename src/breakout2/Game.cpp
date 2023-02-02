#include "Game.hpp"
#include "GlobalsUtil.hpp"
#include "GlobalsGL.hpp"
#include "Input.hpp"

Game::Game(glfw::Window& window)
    : window_{ window }
    , renderer_{ glm::ortho(0.f, 1600.f, 0.f, 900.f) }
    , input_{ window_ }
{
    hook_inputs();

    auto player = registry_.create();
    registry_.emplace<Transform2D>(player, glm::vec2{ 0.f, 40.f }, glm::vec2{ 200.f, 40.f }, 0.f);
    registry_.emplace<Velocity2D>(player, glm::vec2{ 0.f, 0.f });
    registry_.emplace<Sprite>(player, learn::globals::texture_handle_pool.load("src/breakout2/sprites/paddle.png"));
    registry_.emplace<InputMoveComponent>(player, 1000.f);
}


void Game::process_input() {
    using enum InputEvent;

    auto view = registry_.view<const InputMoveComponent, Velocity2D>();

    auto move_func = [&](float velocity_x_sign) {
        view.each(
            [=](const InputMoveComponent& imc, Velocity2D& v) {
                v.velocity.x += velocity_x_sign * imc.max_velocity;
            }
        );
    };

    while (!input_queue_.empty()) {
        auto event = input_queue_.front();

        switch (event) {
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
            default:
                break;
        }

        input_queue_.pop();
    }
}


void Game::update() {
    auto view = registry_.view<Transform2D, const Velocity2D>();

    view.each([](Transform2D& t, const Velocity2D& v) {
        t.position += v.velocity * learn::globals::frame_timer.delta<float>();
    });
}


void Game::render() {
    auto view = registry_.view<const Transform2D, Sprite>();

    view.each([this](const Transform2D& t, Sprite& s) {
        renderer_.draw_sprite(*s.texture, t.mtransform());
    });
}



void Game::hook_inputs() {
    using glfw::KeyCode;

    input_.set_keybind(KeyCode::A, [this](const learn::KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            input_queue_.push(InputEvent::lmove);
        }
        if (args.is_released()) {
            input_queue_.push(InputEvent::lstop);
        }
    });

    input_.set_keybind(KeyCode::D, [this](const learn::KeyCallbackArgs& args) {
        if (args.is_pressed()) {
            input_queue_.push(InputEvent::rmove);
        }
        if (args.is_released()) {
            input_queue_.push(InputEvent::rstop);
        }
    });
}
