#pragma once
#include "All.hpp"
#include "Ball.hpp"
#include "FrameTimer.hpp"
#include "Globals.hpp"
#include "Paddle.hpp"
#include "Sprite.hpp"
#include "GameLevel.hpp"
#include "Rect2D.hpp"
#include "Canvas.hpp"
#include "Transform.hpp"

#include <glm/geometric.hpp>
#include <vector>
#include <cstddef>
#include <iostream>

enum class GameState {
    active, win, menu
};


class Game {
private:
    GameState state_;
    SpriteRenderer& renderer_;
    const learn::FrameTimer& frame_timer_;

    std::vector<GameLevel> levels_;
    size_t current_level_{ 0 };

    static constexpr float player_speed{ 400.f };
    static constexpr float ball_speed{ 400.f };

    Paddle player_{
        Rect2D{ { 400.f, 25.f }, { 150.f, 20.f } }
    };

    Sprite background_{
        learn::globals::texture_handle_pool.load("src/breakout/sprites/background.jpg")
    };

    Ball ball_{
        Circle2D{
            glm::vec2{
                player_.center().x,
                player_.center().y + (player_.size().y / 2.f) + 10.f
            },
            10.f
        }
    };


    struct ControlState {
        bool left{ false };
        bool right{ false };
    };
    ControlState controls_;

public:
    Game(learn::FrameTimer& frame_timer, SpriteRenderer& sprite_renderer)
        : frame_timer_( frame_timer ), renderer_{ sprite_renderer } {}

    Game(SpriteRenderer& sprite_renderer)
        : Game(learn::global_frame_timer, sprite_renderer) {}

    ControlState& controls() noexcept { return controls_; }

    void init() {

        levels_.emplace_back("src/breakout/levels/one.lvl");

        glm::mat4 projection{
            glm::ortho(
                global_canvas.bound_left(),
                global_canvas.bound_right(),
                global_canvas.bound_bottom(),
                global_canvas.bound_top(),
                -1.0f, 1.0f
            )
        };

        renderer_.shader().use().uniform("projection", projection);
    }

    void process_input() {

        const float dx{ player_speed * frame_timer_.delta<float>() };

        auto is_move_in_bounds = [&](float dx) -> bool {
            return glm::abs((player_.center().x + dx) - global_canvas.center.x)
                < (global_canvas.size.x - player_.size().x) / 2.f;
        };


        if (
            (controls_.left && controls_.right) ||
            (!controls_.left && !controls_.right)
        ) {
            player_.velocity().x = 0.f;
            if (ball_.is_stuck()) { ball_.velocity().x = 0.f; }
        } else if (controls_.left && is_move_in_bounds(-dx)) {
            player_.velocity().x = -player_speed;
            if (ball_.is_stuck()) { ball_.velocity().x = -player_speed; }
        } else if (controls_.right && is_move_in_bounds(dx)) {
            player_.velocity().x = +player_speed;
            if (ball_.is_stuck()) { ball_.velocity().x = +player_speed; }
        } else {
            player_.velocity().x = 0.f;
            if (ball_.is_stuck()) { ball_.velocity().x = 0.f; }
        }

    }

    void update() {
        update_player_movement();
        update_ball_movement();
    }


    void launch_ball() {
        if (ball_.is_stuck()) {
            ball_.make_unstuck();
            ball_.velocity() =
                ball_speed * glm::normalize(player_.velocity() + glm::vec2{ 0.f, 400.f });
        }
    }

    void render() {

        renderer_.draw_sprite(
            background_,
            learn::Transform()
                .translate({ global_canvas.center.x, global_canvas.center.y, 0.f })
                .scale({ global_canvas.width(), global_canvas.height(), 1.f })
        );

        for (auto&& tile : levels_[current_level_].tiles()) {
            renderer_.draw_sprite(tile.sprite(), tile.get_transform());
        }

        renderer_.draw_sprite(player_.sprite(), player_.get_transform());
        renderer_.draw_sprite(ball_.sprite(), ball_.get_transform());

    }


private:
    void update_player_movement() {
        player_.center() += player_.velocity() * frame_timer_.delta<float>();
    }

    void update_ball_movement() {

        // Super messy
        const auto dxdy = ball_.velocity() * frame_timer_.delta<float>();
        const auto new_pos = ball_.center() + dxdy;

        if (!global_canvas.contains(new_pos)) {
            // std::clog << "Out of Bounds on Next Step\n";

            if (new_pos.x + ball_.radius() > global_canvas.bound_right()) {
                // std::clog << "Collision Right\n";
                ball_.velocity().x =  -ball_.velocity().x;
            } else if (new_pos.x - ball_.radius() < global_canvas.bound_left()) {
                // std::clog << "Collision Left\n";
                ball_.velocity().x = -ball_.velocity().x;
            }

            if (new_pos.y + ball_.radius() > global_canvas.bound_top()) {
                // std::clog << "Collision Top\n";
                ball_.velocity().y = -ball_.velocity().y;

            } else if (new_pos.y - ball_.radius() < global_canvas.bound_bottom()) {
                // std::clog << "Collision Bottom\n";
                ball_.velocity().y = -ball_.velocity().y;
                // FIXME: Lose the game here.
                // Maybe return false from a function.
            }

        } else {
            ball_.center() = new_pos;
        }
    }

};
