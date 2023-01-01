#pragma once
#include "All.hpp"
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
        learn::Transform()
            .translate({ 400.f - 75.f, 575.f - 20.f, 0.f })
            .scale({ 150.f, 40.f, 0.f })
    };

    Sprite background_{
        learn::globals::texture_handle_pool.load("src/breakout/sprites/background.jpg")
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

        GameLevel level{
            GameLevel::tilemap_from_text(
                "1 0 1 2 0 1\n"
                "0 0 2 2 0 0\n"
                "3 3 0 0 3 3\n"
                "4 5 1 1 5 4\n"
            )
        };
        levels_.emplace_back(std::move(level));

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
        constexpr float velocity{ 5.0f };
        // FIXME: do bound checking
        if (controls_.left) {
            player_.transform().translate({ -velocity * frame_timer_.delta(), 0.f, 0.f });
        }
        if (controls_.right) {
            player_.transform().translate({ +velocity * frame_timer_.delta(), 0.f, 0.f });
        }
    }

    void update();
    void render() {

        renderer_.draw_sprite(
            background_,
            learn::Transform()
                .scale({ global_canvas.width(), global_canvas.height(), 1.f })
        );

        for (auto&& tile : levels_[current_level_].tiles()) {
            renderer_.draw_sprite(tile.sprite(), tile.transform());
        }

        renderer_.draw_sprite(player_.sprite(), player_.transform());

    }

};
