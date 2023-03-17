#include "Game.hpp"
#include "Events.hpp"
#include "Tile.hpp"
#include "Transform2D.hpp"


void Game::process_events() {
    process_input_events();
    process_tile_collision_events();
    process_fx_state_updates();
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

        auto tile_entity = event.tile_entity;

        const TileType type = registry_.get<TileComponent>(tile_entity).type;

        if (type != TileType::solid) {
            const glm::vec2 position = registry_.get<Transform2D>(tile_entity).position;

            trash_.destroy_later(tile_entity);

            // FIXME: Maybe send a 'create powerup' event instead?
            powerup_generator_.try_generate_random_at(registry_, physics_, position);
        }
    }
}


void Game::process_fx_state_updates() {
    while (!events.fx_toggle.empty()) {
        auto event = events.fx_toggle.pop();


        switch (event.type) {
            case FXType::speed:
                {
                    auto& imc = registry_.get<InputMoveComponent>(player_);
                    if (event.toggle_type == FXToggleType::enable) {
                        imc.max_velocity = player_base_speed * 1.3f;
                    } else {
                        imc.max_velocity = player_base_speed;
                    }
                }
                break;
            default:
                break;
        }
    }

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


