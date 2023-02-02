#include "Game.hpp"
#include "GlobalsGL.hpp"
#include "Input.hpp"

Game::Game(glfw::Window& window)
    : window_{ window }
    , renderer_{ glm::ortho(0.f, 1600.f, 0.f, 900.f, -1.f, 1.f) }
    , input_{ window_ }
{
    hook_inputs();
    init_registry();
    levels_.emplace_back(GameLevel::from_file("src/breakout2/levels/one.lvl"));
    current_level().build_level_entities(registry_);
}


void Game::process_input() {

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
    auto view = registry_.view<Transform2D, const Velocity2D>();

    view.each([](Transform2D& t, const Velocity2D& v) {
        t.position += v.velocity * update_time_step;
    });
}


void Game::render() {
    renderer_.draw_sprites(registry_);
}



void Game::launch_ball() {
    // This is scuffed in a way, but sort of makes sense.
    if (registry_.remove<InputMoveComponent>(ball_)) {

        auto& player_v = registry_.get<const Velocity2D>(player_);

        registry_.patch<Velocity2D>(ball_, [&](Velocity2D& v) {
            v.velocity = ball_base_speed * glm::normalize(glm::vec2{ 0.f, ball_base_speed } + player_v.velocity);
        });

    }
}


void Game::init_registry() {

    player_ = registry_.create();
    registry_.emplace<Transform2D>(player_, glm::vec2{ 800.f, 30.f }, glm::vec2{ 200.f, 30.f }, 0.f);
    registry_.emplace<Velocity2D>(player_, glm::vec2{ 0.f, 0.f });
    registry_.emplace<Sprite>(player_,
        learn::globals::texture_handle_pool.load("src/breakout2/sprites/paddle.png"),
        ZDepth::foreground
    );
    registry_.emplace<InputMoveComponent>(player_, player_base_speed);

    ball_ = registry_.create();
    registry_.emplace<Transform2D>(ball_, glm::vec2{ 800.f, 30.f + 30.f }, glm::vec2{ 30.f, 30.f }, 0.f);
    registry_.emplace<Velocity2D>(ball_, glm::vec2{ 0.f, 0.f });
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
