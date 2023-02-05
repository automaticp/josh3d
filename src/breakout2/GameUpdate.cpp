#include "Game.hpp"
#include "PhysicsSystem.hpp"
#include "Transform2D.hpp"
#include "Canvas.hpp"
#include <imgui.h>

void Game::update() {

    update_player_velocity();

    physics_.update(update_time_step);

    update_transforms();

    // This does not work...
    // auto& p = registry_.get<PhysicsComponent>(ball_);
    // p.set_velocity(ball_base_speed * glm::normalize(p.get_velocity()));

    update_fx_state();
}

void Game::update_player_velocity() {
    auto [phys, trans, imc] = registry_.get<PhysicsComponent, Transform2D, const InputMoveComponent>(player_);

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


void Game::update_fx_state() {
    fx_manager_.update(update_time_step);
}


void Game::update_gui() {
    // Sprite& s = registry_.get<Sprite>(player_);
    // ImGui::Begin("Debug");
    // ImGui::SliderFloat("Paddle Depth", &s.depth, -2.f, 2.f);
    // ImGui::End();
}
