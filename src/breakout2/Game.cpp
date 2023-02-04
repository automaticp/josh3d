#include "Game.hpp"
#include "GlobalsGL.hpp"
#include "Input.hpp"
#include "Events.hpp"
#include "PhysicsSystem.hpp"
#include "ImGuiContextWrapper.hpp"
#include "Tile.hpp"
#include <glm/geometric.hpp>
#include <imgui.h>

Game::Game(glfw::Window& window)
    : window_{ window }
    , renderer_{ glm::ortho(0.f, 1600.f, 0.f, 900.f, -1.f, 1.f) }
    , physics_{ registry_ }
    , input_{ window_ }
    , imgui_{ window_ }
{
    hook_inputs();
    init_registry();
    levels_.emplace_back(GameLevel::from_file("src/breakout2/levels/one.lvl"));
    current_level().build_level_entities(registry_, physics_);
    init_walls();
    // Ewww, sticky
    sticky_joint_ = physics_.weld(player_, ball_);
}


void Game::process_events() {
    process_input_events();
    process_tile_collision_events();
}


void Game::process_input_events() {

    auto& imc = registry_.get<InputMoveComponent>(player_);

    while (!events.input.empty()) {
        auto event = events.input.pop();

        switch (event) {
            using enum InputEvent;
            case lmove:
                imc.wants_move_left = true;
                break;
            case lstop:
                imc.wants_move_left = false;
                break;
            case rmove:
                imc.wants_move_right = true;
                break;
            case rstop:
                imc.wants_move_right = false;
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
    }
}

void Game::process_tile_collision_events() {
    while (!events.tile_collision.empty()) {
        auto event = events.tile_collision.pop();

        const TileType type = registry_.get<TileComponent>(event.tile_entity).type;
        if (type != TileType::solid) {
            registry_.destroy(event.tile_entity);
        }
    }
}


void Game::update() {

    update_player_velocity();

    physics_.update(update_time_step);

    update_transforms();

    // This does not work...
    // auto& p = registry_.get<PhysicsComponent>(ball_);
    // p.set_velocity(ball_base_speed * glm::normalize(p.get_velocity()));
}

void Game::update_player_velocity() {
    auto [phys, trans, imc] = registry_.get<PhysicsComponent, Transform2D, InputMoveComponent>(player_);

    float input_speed{ 0.f };

    if (imc.wants_move_left) {
        input_speed -= imc.max_velocity;
    }

    if (imc.wants_move_right) {
        input_speed += imc.max_velocity;
    }

    // The pushback velocity is the most sane way I've found to handle
    // paddle collision with the canvas borders.
    //
    // Resetting position when alredy OOB will definetly break the physics
    // as you will appear stationary at the edge but with a nonzero velocity.
    // (Do not 'teleport' objects if you want to work *together* with the
    // physics engine)
    //
    // Multiplicatively dampening current velocity also breaks down
    // when you're not moving and the (1 / step) blows up to infinity.

    const float lim_right = canvas.bound_right() - trans.scale.x / 2.f;
    const float lim_left  = canvas.bound_left()  + trans.scale.x / 2.f;

    const float current_pos{ trans.position.x };
    const float next_pos{ current_pos + input_speed * update_time_step };

    // Push back when close to the borders to not penetrate.
    // This will also push you back even if you're not pressing move
    // and will get you unstuck if the paddle grows in size near the border.
    float pushback_speed{ 0.0f };

    if (next_pos > lim_right) {
        pushback_speed = (lim_right - next_pos) / update_time_step;
    } else if (next_pos < lim_left) {
        pushback_speed = (lim_left - next_pos) / update_time_step;
    }

    phys.set_velocity({ pushback_speed + input_speed, 0.f });

}


void Game::update_transforms() {
    auto view = registry_.view<const PhysicsComponent, Transform2D>();

    for (auto [entity, phys, trans] : view.each()) {
        trans.position = to_screen(phys.body->GetPosition());
    }
}



void Game::update_gui() {
    Sprite& s = registry_.get<Sprite>(player_);

    ImGui::Begin("Debug");

    ImGui::SliderFloat("Paddle Depth", &s.depth, -2.f, 2.f);

    ImGui::End();
}




void Game::render() {
    imgui_.new_frame();

    renderer_.draw_sprites(registry_);

    update_gui();

    imgui_.render();
}



void Game::launch_ball() {
    // TODO: move the joint into something better than a raw ptr member var
    if (sticky_joint_) {
        physics_.unweld(sticky_joint_);
        sticky_joint_ = nullptr;

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
    const glm::vec2 ball_pos{ player_pos + glm::vec2{ 0.f, player_scale.y / 2.f + ball_scale.y / 2.f } };

    registry_.emplace<Transform2D>(ball_, ball_pos, ball_scale, 0.f);
    registry_.emplace<PhysicsComponent>(ball_, physics_.create_ball(ball_, ball_pos, ball_scale.x / 2.f));
    registry_.emplace<Sprite>(ball_,
        learn::globals::texture_handle_pool.load("src/breakout2/sprites/awesomeface.png"),
        ZDepth::foreground
    );
    // registry_.emplace<InputMoveComponent>(ball_, player_base_speed);

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
}
