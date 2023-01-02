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

    Paddle player_{
        Rect2D{ { 400.f, 575.f }, { 150.f, 40.f } }
    };

    Sprite background_{
        learn::globals::texture_handle_pool.load("src/breakout/sprites/background.jpg")
    };

    Ball ball_{
        Circle2D{ glm::vec2{400.f, 575.f - 20.f - 30.f }, 30.f }
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

        // GameLevel level{
        //     GameLevel::tilemap_from_text(
        //         "1 0 1 2 0 1\n"
        //         "0 0 2 2 0 0\n"
        //         "3 3 0 0 3 3\n"
        //         "4 5 1 1 5 4\n"
        //     )
        // };

        levels_.emplace_back("src/breakout/levels/one.lvl");

        // Figure out this line later
        glm::mat4 projection{
            glm::ortho(
                global_canvas.bound_left(),
                global_canvas.bound_right(),
                global_canvas.bound_top(),     // Inverted coordinates,
                global_canvas.bound_bottom(),  // Making the top-left corner (0, 0).
                                               // That's actually pretty bad...
                -1.0f, 1.0f
            )
        };

        renderer_.shader().use().uniform("projection", projection);
    }

    void process_input() {
        constexpr float velocity{ 400.0f };
        const float dx{ velocity * frame_timer_.delta<float>() };

        auto is_move_in_bounds = [&](float dx) -> bool {
            return glm::abs((player_.center().x + dx) - global_canvas.center.x)
                < (global_canvas.size.x - player_.size().x) / 2.f;
        };

        if (controls_.left && is_move_in_bounds(-dx)) {
            player_.center().x -= dx;
            if (ball_.is_stuck()) {
                ball_.center().x -= dx;
            }
        }
        if (controls_.right && is_move_in_bounds(+dx)) {
            player_.center().x += dx;
            if (ball_.is_stuck()) {
                ball_.center().x += dx;
            }
        }
    }

    void update() {
        update_ball_movement();
    }


    void launch_ball() {
        if (ball_.is_stuck()) {
            ball_.make_unstuck();
            // FIXME: make the velocity dependant on movement
            ball_.velocity() = 400.f * glm::vec2{ 0.7f, -0.2f };
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
    void update_ball_movement() {

        if (ball_.is_stuck()) {
            return;
        }


        // Super messy
        const auto dxdy = ball_.velocity() * frame_timer_.delta<float>();
        const auto new_pos = ball_.center() + dxdy;

        if (!global_canvas.contains(new_pos)) {
            // std::clog << "Out of Bounds on Next Step\n";

            if (new_pos.x + ball_.radius() > global_canvas.bound_right()) {
                // std::clog << "Collision Right\n";
                ball_.velocity().x =  -ball_.velocity().x;
                // ball_.center().x = global_canvas.bound_right();
            } else if (new_pos.x - ball_.radius() < global_canvas.bound_left()) {
                // std::clog << "Collision Left\n";
                ball_.velocity().x = -ball_.velocity().x;
                // ball_.center().x = global_canvas.bound_left();
            }

            // Bottom is top because
            if (new_pos.y - ball_.radius() < global_canvas.bound_bottom()) {
                // std::clog << "Collision Top\n";
                ball_.velocity().y = -ball_.velocity().y;
                // ball_.center().y = global_canvas.bound_top();
            } else if (new_pos.y + ball_.radius() > global_canvas.bound_top()) {
                // std::clog << "Collision Bottom\n";
                ball_.velocity().y = -ball_.velocity().y;
                // FIXME: Lose the game here.
                // Maybe return false from a function.
            }

            // ball_.center() += ball_.velocity() * frame_timer_.delta<float>();
        } else {
            ball_.center() = new_pos;
        }
    }

};
