#pragma once
#include "All.hpp"
#include "Ball.hpp"
#include "Collisions.hpp"
#include "FrameTimer.hpp"
#include "Globals.hpp"
#include "Paddle.hpp"
#include "Particle2D.hpp"
#include "Particle2DGenerator.hpp"
#include "Sprite.hpp"
#include "GameLevel.hpp"
#include "Rect2D.hpp"
#include "Canvas.hpp"
#include "Transform.hpp"

#include <array>
#include <glbinding/gl/enum.h>
#include <glm/common.hpp>
#include <random>
#include <range/v3/all.hpp>
#include <glm/fwd.hpp>
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

    static constexpr float player_speed{ 500.f };
    static constexpr float ball_speed{ 500.f };

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

    Particle2DGenerator particle_gen_{
        100u,
        Sprite{
            learn::globals::texture_handle_pool.load("src/breakout/sprites/particle_white.png")
        },
        std::normal_distribution<float>{ 0.7f, 0.15f },
        glm::vec2{ ball_.center() },
        glm::vec2{ ball_.radius() / 4.f, ball_.radius() / 4.f },
        glm::vec4{ 0.f, 0.9f, 0.6f, 0.9f }
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

        constexpr float drag{ 2.f };
        particle_gen_.set_origin(ball_.center() - drag * (ball_.velocity() * frame_timer_.delta<float>()));

        update_ball_movement();

        particle_gen_.update(frame_timer_.delta<float>(), ball_.velocity());

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
            if (tile.is_alive()) {
                renderer_.draw_sprite(tile.sprite(), tile.get_transform());
            }
        }

        renderer_.draw_sprite(player_.sprite(), player_.get_transform());

        using namespace gl;
        if (!ball_.is_stuck()) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            for (const Particle2D& p : particle_gen_.particles()) {
                if (p.lifetime > 0.f) {
                    renderer_.draw_sprite(particle_gen_.sprite(), p.get_transform(), p.color);
                }
            }
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        renderer_.draw_sprite(ball_.sprite(), ball_.get_transform());

    }


private:
    void resolve_collisions() {
        for (auto&& tile : levels_[current_level_].tiles()) {
            if (tile.is_alive()){
                if (check_collision(tile.box(), ball_.circle())) {
                    if (tile.type() != TileType::solid) {
                        tile.destroy();
                    }
                    // do more stuff
                }
            }
        }
    }


    void update_player_movement() {
        player_.center() += player_.velocity() * frame_timer_.delta<float>();
    }

    void update_ball_movement() {

        // Super messy
        auto dxdy = ball_.velocity() * frame_timer_.delta<float>();

        enum class RectXCollisionType {
            none, left, right
        };
        enum class RectYCollisionType {
            none, top, bottom
        };

        struct InnerRectCollisionInfo {
            RectXCollisionType x_collision{ RectXCollisionType::none };
            RectYCollisionType y_collision{ RectYCollisionType::none };
            glm::vec2 overshoot{ 0.f, 0.f };

            bool did_collide() const noexcept {
                return x_collision != RectXCollisionType::none
                    || y_collision != RectYCollisionType::none;
            }
        };

        auto inner_ball_on_rect_collision =
            [](const Rect2D& rect, const Ball& ball, glm::vec2 dxdy)
                -> InnerRectCollisionInfo
            {

                const glm::vec2 new_pos = ball.center() + dxdy;
                const glm::vec2 move_direction = glm::sign(dxdy);
                const glm::vec2 new_edge_pos = new_pos + move_direction * ball.radius();

                InnerRectCollisionInfo info{};
                auto& [x_collision, y_collision, overshoot] = info;

                if (new_edge_pos.x > rect.bound_right()) {

                    overshoot.x = new_edge_pos.x - rect.bound_right();
                    x_collision = RectXCollisionType::right;

                } else if (new_edge_pos.x < rect.bound_left()) {

                    overshoot.x = new_edge_pos.x - rect.bound_left();
                    x_collision = RectXCollisionType::left;

                }


                if (new_edge_pos.y > rect.bound_top()) {

                    overshoot.y = new_edge_pos.y - rect.bound_top();
                    y_collision = RectYCollisionType::top;

                } else if (new_edge_pos.y < rect.bound_bottom()) {

                    overshoot.y = new_edge_pos.y - rect.bound_bottom();
                    y_collision = RectYCollisionType::bottom;

                }

                return info;
            };

        // Resolve the collision with the canvas edges
        auto canvas_collision = inner_ball_on_rect_collision(global_canvas, ball_, dxdy);
        if (canvas_collision.did_collide()) {
            if (canvas_collision.x_collision != RectXCollisionType::none) {
                ball_.velocity().x = -ball_.velocity().x;
            }
            if (canvas_collision.y_collision != RectYCollisionType::none) {
                ball_.velocity().y = -ball_.velocity().y;
            }
            dxdy = dxdy - (2.f * canvas_collision.overshoot);

            std::clog << "Canvas Overshoot:   (" << canvas_collision.overshoot.x << ", "
                << canvas_collision.overshoot.y << ")\n";
        }


        // The side of the rectangle
        // that the ball collided with,
        // or 'none' if no collision occured.
        enum class RectCollisionType : size_t {
            none=0, left, right, top, bottom
        };


        auto outer_collision_direction =
            [](glm::vec2 difference_vector) -> RectCollisionType {
                constexpr std::array<glm::vec2, 4> compass{ {
                    { -1.f, 0.f }, // left
                    { 1.f, 0.f },  // right
                    { 0.f, 1.f },  // top
                    { 0.f, -1.f }  // bottom
                } };

                auto me = ranges::max_element(compass, {},
                    [&](const glm::vec2& dir) {
                        return glm::dot(dir, -difference_vector);
                    }
                );
                size_t elem_idx = ranges::distance(ranges::begin(compass), me);
                return RectCollisionType{ elem_idx + 1 };
            };

        struct OuterRectCollisionInfo {
            RectCollisionType type{ RectCollisionType::none };
            glm::vec2 difference{ 0.f, 0.f };

            bool did_collide() const noexcept {
                return type != RectCollisionType::none;
            }
        };

        auto outer_ball_on_rect_collision =
            [&](const Rect2D& rect, const Ball& ball, glm::vec2 dxdy)
                -> OuterRectCollisionInfo
            {

                const glm::vec2 new_ball_center =
                    ball.center() + dxdy;

                const glm::vec2 center_difference =
                    new_ball_center - rect.center;

                const glm::vec2 clamped =
                    glm::clamp(
                        center_difference,
                        -rect.half_size(),
                        rect.half_size()
                    );

                const glm::vec2 closest = rect.center + clamped;
                const glm::vec2 difference = closest - new_ball_center;

                bool overlaps =
                    glm::length(difference) < ball.radius();

                if (overlaps) {
                    return {
                        .type=outer_collision_direction(difference),
                        .difference=difference
                    };
                } else {
                    return {};
                }
            };



        auto apply_outer_collision_correction =
            [](Ball& ball, glm::vec2& dxdy, const OuterRectCollisionInfo& collision) {

                const auto&[dir, diff_vector] = collision;

                if (dir == RectCollisionType::left ||
                    dir == RectCollisionType::right) {

                        ball.velocity().x *= -1.f;

                        const float penetration =
                            ball.radius() - glm::abs(diff_vector.x);

                        std::clog << "Tile X Penetration: " << penetration << '\n';

                        if (dir == RectCollisionType::left) {
                            dxdy.x -= 2.f * penetration;
                        } else {
                            dxdy.x += 2.f * penetration;
                        }
                    } else /* vertical */ {

                        ball.velocity().y *= -1.f;

                        const float penetration =
                            ball.radius() - glm::abs(diff_vector.y);

                        std::clog << "Tile Y Penetration: " << penetration << '\n';

                        if (dir == RectCollisionType::top) {
                            dxdy.y += 2.f * penetration;
                        } else {
                            dxdy.y -= 2.f * penetration;
                        }
                    }

            };



        // Resolve collisions with tiles
        for (auto&& tile : levels_[current_level_].tiles()) {
            if (tile.is_alive()) {
                auto tile_collision =
                    outer_ball_on_rect_collision(tile.box(), ball_, dxdy);

                if (tile_collision.did_collide()) {
                    if (tile.type() != TileType::solid) {
                        tile.destroy();
                    }

                    apply_outer_collision_correction(ball_, dxdy, tile_collision);
                }

            }
        }

        if (!ball_.is_stuck()) {

            auto paddle_collision =
                outer_ball_on_rect_collision(
                    player_.box(), ball_, dxdy
                );

            if (paddle_collision.did_collide()) {
                apply_outer_collision_correction(ball_, dxdy, paddle_collision);

                ball_.velocity() = ball_speed * glm::normalize(ball_.velocity() + player_.velocity());

                ball_.velocity().y = glm::abs(ball_.velocity().y);
            }

        }

        ball_.center() += dxdy;
    }

};
