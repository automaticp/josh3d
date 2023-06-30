#pragma once
#include "Ball.hpp"
#include "Collisions.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "GlobalsGL.hpp"
#include "Input.hpp"
#include "Logging.hpp"
#include "Paddle.hpp"
#include "Particle2D.hpp"
#include "Particle2DGenerator.hpp"
#include "PowerUp.hpp"
#include "PowerUpGenerator.hpp"
#include "ShaderBuilder.hpp"
#include "Sprite.hpp"
#include "GameLevel.hpp"
#include "Canvas.hpp"
#include "Transform.hpp"
#include "SpriteRenderer.hpp"
#include "PostprocessDoubleBuffer.hpp"
#include "PostprocessRenderer.hpp"
#include "FXState.hpp"

#include <array>
#include <glbinding/gl/enum.h>
#include <glm/common.hpp>
#include <random>
#include <range/v3/all.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <type_traits>
#include <vector>
#include <cstddef>
#include <iostream>
#include <cassert>

enum class GameState {
    active, win, menu
};



class Game {
private:
    glfw::Window& window_;

    GameState state_;
    SpriteRenderer renderer_{
        learn::ShaderSource::from_file("src/breakout/shaders/sprite.vert"),
        learn::ShaderSource::from_file("src/breakout/shaders/sprite.frag")
    };

    learn::PostprocessDoubleBuffer ppdb_;
    learn::PostprocessRenderer pp_renderer_;

    learn::ShaderProgram pp_shake_{
        learn::ShaderBuilder()
            .load_vert("src/breakout/shaders/pp_shake.vert")
            .load_frag("src/breakout/shaders/pp_kernel_blur.frag")
            .get()
    };

    learn::ShaderProgram pp_chaos_{
        learn::ShaderBuilder()
            .load_vert("src/breakout/shaders/pp_chaos.vert")
            .load_frag("src/breakout/shaders/pp_kernel_edge.frag")
            .get()
    };


    learn::ShaderProgram pp_confuse_{
        learn::ShaderBuilder()
            .load_vert("src/breakout/shaders/pp_confuse.vert")
            .load_frag("src/breakout/shaders/pp_invert.frag")
            .get()
    };

    FXState fx_;

    PowerUpGenerator powerup_gen_;

    const learn::FrameTimer& frame_timer_;

    std::vector<GameLevel> levels_;
    size_t current_level_{ 0 };

    static constexpr float base_player_speed{ 500.f };
    static constexpr float ball_speed{ 500.f };

    static constexpr float paddle_width_default{ 130.f };
    static constexpr float paddle_width_enhanced{ 160.f };

    Paddle player_{
        Rect2D{ { 400.f, 20.f }, { paddle_width_default, 20.f } }
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


    learn::BasicRebindableInput input_;

    struct ControlState {
        bool left{ false };
        bool right{ false };
    };
    ControlState controls_;

public:
    Game(glfw::Window& window, learn::FrameTimer& frame_timer)
        : window_{ window }
        , ppdb_{
            learn::globals::window_size.width(),
            learn::globals::window_size.height()
        }
        , frame_timer_{ frame_timer }
        , input_{ window_ }
    {}

    Game(glfw::Window& window)
        : Game(window, learn::globals::frame_timer)
    {}


    void init() {

        using namespace gl;

        init_input();

        window_.framebufferSizeEvent.setCallback(
            [this](glfw::Window&, int w, int h) {
                learn::globals::window_size.set_to(w, h);
                glViewport(0, 0, w, h);
                ppdb_.reset_size(w, h);
            }
        );

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

        const float speed_modifier = fx_.is_active(FXType::speed) ? 1.2f : 1.0f;

        const float player_speed = base_player_speed * speed_modifier;

        const float dx{ player_speed * frame_timer_.delta<float>() };

        // FIXME: If you pick up the paddle size increase
        // powerup while standing near the wall, you get stuck.
        //
        // Have fun!
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
            if (ball_.is_stuck()) { ball_.velocity().x = player_.velocity().x; }
        } else if (controls_.right && is_move_in_bounds(dx)) {
            player_.velocity().x = +player_speed;
            if (ball_.is_stuck()) { ball_.velocity().x = player_.velocity().x; }
        } else {
            player_.velocity().x = 0.f;
            if (ball_.is_stuck()) { ball_.velocity().x = 0.f; }
        }

    }

    void update() {

        update_player_movement();

        update_powerup_movement();

        powerup_gen_.remove_destroyed();

        constexpr float drag{ 2.f };
        particle_gen_.set_origin(ball_.center() - drag * (ball_.velocity() * frame_timer_.delta<float>()));

        update_ball_movement();

        particle_gen_.update(frame_timer_.delta<float>(), ball_.velocity());

        fx_.update(frame_timer_.delta<float>());

    }

    void launch_ball() {
        if (ball_.is_stuck()) {
            ball_.make_unstuck();
            ball_.velocity() =
                ball_speed * glm::normalize(player_.velocity() + glm::vec2{ 0.f, 400.f });
            fx_.enable(FXType::shake, 0.05f);
        }
    }

    void render() {
        using namespace gl;

        ppdb_.back().framebuffer()
            .bind_as(GL_DRAW_FRAMEBUFFER)
            .and_then([this] { draw_scene_objects(); })
            .unbind();
        ppdb_.swap_buffers();


        auto render_pp = [this](learn::ActiveShaderProgram& asp) {
            ppdb_.back().framebuffer()
                .bind_as(GL_DRAW_FRAMEBUFFER)
                .and_then([this, &asp] {
                    pp_renderer_.draw(asp, ppdb_.front_target());
                })
                .unbind();
            ppdb_.swap_buffers();
        };

        if (fx_.is_active(FXType::chaos)) {

            // Ughh...
            GLenum old_wrap;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &old_wrap);
            auto& target = ppdb_.front_target();

            target.bind()
                .set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT)
                .set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

            auto asp = pp_chaos_.use();
            asp.uniform("time", frame_timer_.current<float>());
            render_pp(asp);

            target.bind()
                .set_parameter(GL_TEXTURE_WRAP_S, old_wrap)
                .set_parameter(GL_TEXTURE_WRAP_T, old_wrap);


        }

        if (fx_.is_active(FXType::confuse)) {

            auto asp = pp_confuse_.use();
            render_pp(asp);

        }

        if (fx_.is_active(FXType::shake)) {

            auto asp = pp_shake_.use();
            asp.uniform("time", frame_timer_.current<float>());
            render_pp(asp);

        }


        auto [w, h] = learn::globals::window_size.size();

        learn::BoundFramebuffer::unbind_as(GL_DRAW_FRAMEBUFFER);
        ppdb_.front().framebuffer()
            .bind_as(GL_READ_FRAMEBUFFER)
            .blit(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST)
            .unbind();

    }


private:
    void draw_scene_objects() {

        renderer_.draw_sprite(
            background_,
            learn::MTransform()
                .translate({ global_canvas.center.x, global_canvas.center.y, 0.f })
                .scale({ global_canvas.width(), global_canvas.height(), 1.f })
        );

        for (auto& tile : levels_[current_level_].tiles()) {
            if (tile.is_alive()) {
                renderer_.draw_sprite(tile.sprite(), tile.get_transform());
            }
        }

        for (auto& pup : powerup_gen_.powerups()) {
            if (pup.is_alive()) {
                renderer_.draw_sprite(pup.sprite(), pup.get_transform());
            }
        }

        renderer_.draw_sprite(
            player_.sprite(), player_.get_transform(),
            fx_.is_active(FXType::sticky) ? glm::vec4{ 0.2f, 1.f, 0.5f, 1.f } : player_.sprite().color()
        );

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

        renderer_.draw_sprite(
            ball_.sprite(), ball_.get_transform(),
            fx_.is_active(FXType::pass_through) ? glm::vec4{ 1.f, 0.5f, 0.5f, 1.f } : ball_.sprite().color()
        );

    }


    void update_player_movement() {
        player_.size().x =
            fx_.is_active(FXType::pad_size_up) ? paddle_width_enhanced : paddle_width_default;

        player_.center() += player_.velocity() * frame_timer_.delta<float>();
    }

    void update_powerup_movement() {
        for (auto& pup : powerup_gen_.powerups()) {
            pup.box().center += pup.velocity() * frame_timer_.delta<float>();
            if (pup.is_alive()) {
                if (check_collision(pup.box(), player_.box())) {
                    // Apply effect...
                    apply_powerup(pup.type());
                    pup.destroy();
                    learn::globals::logstream << "Destroyed PowerUp Type " << size_t(pup.type()) << " On Player Collision\n";
                } else if (!check_collision(pup.box(), global_canvas)) {
                    // Just destroy
                    pup.destroy();
                    learn::globals::logstream << "Destroyed PowerUp Type " << size_t(pup.type()) << " On Out-of-Bounds\n";
                }
            }
        }
    }

    void apply_powerup(PowerUpType type) {
        switch (type) {
            case PowerUpType::chaos:
                if (!fx_.is_active(FXType::confuse)) {
                    fx_.enable(FXType::chaos, 5.f);
                }
                break;
            case PowerUpType::confuse:
                if (!fx_.is_active(FXType::chaos)) {
                    fx_.enable(FXType::confuse, 10.f); break;
                }
                break;
            case PowerUpType::pad_size_up:  fx_.enable(FXType::pad_size_up, 30.f); break;
            case PowerUpType::pass_through: fx_.enable(FXType::pass_through, 10.f); break;
            case PowerUpType::speed:        fx_.enable(FXType::speed, 30.f); break;
            case PowerUpType::sticky:       fx_.enable(FXType::sticky, 20.f); break;
            default: break;
        }
    }

    void update_ball_movement() {

        auto dxdy = ball_.velocity() * frame_timer_.delta<float>();

        // Resolve the collision with the canvas edges
        auto canvas_collision =
            inner_ball_on_rect_collision(global_canvas, ball_, dxdy);
        if (canvas_collision.did_collide()) {
            if (canvas_collision.x_collision != RectXCollisionType::none) {
                ball_.velocity().x = -ball_.velocity().x;
            }
            if (canvas_collision.y_collision != RectYCollisionType::none) {
                ball_.velocity().y = -ball_.velocity().y;
            }
            dxdy = dxdy - (2.f * canvas_collision.overshoot);
        }


        // Resolve collisions with tiles
        for (auto&& tile : levels_[current_level_].tiles()) {
            if (tile.is_alive()) {
                auto tile_collision =
                    outer_ball_on_rect_collision(tile.box(), ball_, dxdy);

                if (tile_collision.did_collide()) {
                    if (tile.type() != TileType::solid) {
                        powerup_gen_.try_generate_random_at(tile.box().center);
                        tile.destroy();
                        levels_[current_level_].report_destroyed_tile();
                        if (!fx_.is_active(FXType::pass_through)) {
                            apply_outer_collision_correction(ball_, dxdy, tile_collision);
                        }
                    } else {
                        fx_.enable(FXType::shake, 0.05f);
                        apply_outer_collision_correction(ball_, dxdy, tile_collision);
                    }

                }

            }
        }

        // Resolve the collision with the paddle
        if (!ball_.is_stuck()) {

            auto paddle_collision =
                outer_ball_on_rect_collision(
                    player_.box(), ball_, dxdy
                );

            if (paddle_collision.did_collide()) {
                if (fx_.is_active(FXType::sticky)) {
                    ball_.make_stuck();
                    return;
                }

                apply_outer_collision_correction(ball_, dxdy, paddle_collision);

                ball_.velocity() = ball_speed * glm::normalize(ball_.velocity() + player_.velocity());

                ball_.velocity().y = glm::abs(ball_.velocity().y);

                fx_.enable(FXType::shake, 0.03f);
            }

        }

        ball_.center() += dxdy;
    }



    void init_input() {
        input_.set_keybind(
            glfw::KeyCode::A,
            [this](const learn::KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Press) { controls_.left = true; }
                if (args.state == glfw::KeyState::Release) { controls_.left = false; }
            }
        );
        input_.set_keybind(
            glfw::KeyCode::D,
            [this](const learn::KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Press) { controls_.right = true; }
                if (args.state == glfw::KeyState::Release) { controls_.right = false; }
            }
        );
        input_.set_keybind(
            glfw::KeyCode::Space,
            [this](const learn::KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Press) { launch_ball(); }
            }
        );
        input_.set_keybind(
            glfw::KeyCode::Escape,
            [this](const learn::KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    window_.setShouldClose(true);
                }
            }
        );

        input_.set_keybind(
            glfw::KeyCode::H,
            [this](const learn::KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Press) {
                    fx_.enable(FXType::shake, 0.05f);
                }
            }
        );

        input_.set_keybind(
            glfw::KeyCode::G,
            [this](const learn::KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Press) {
                    fx_.enable(FXType::chaos, 5.f);
                }
            }
        );


        input_.enable_key_callback();

    }

};
