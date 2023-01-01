#pragma once
#include "All.hpp"
#include "FrameTimer.hpp"
#include "Globals.hpp"
#include "Sprite.hpp"
#include "GameLevel.hpp"
#include "Rect2D.hpp"
#include "Canvas.hpp"

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

public:
    Game(learn::FrameTimer& frame_timer, SpriteRenderer& sprite_renderer)
        : frame_timer_( frame_timer ), renderer_{ sprite_renderer } {}

    Game(SpriteRenderer& sprite_renderer)
        : Game(learn::global_frame_timer, sprite_renderer) {}

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
                global_canvas.bound_top(),  // Inverted coordinates,
                global_canvas.bound_bottom(),  // Making the top-left corner (0, 0)
                -1.0f, 1.0f
            )
        };

        renderer_.shader().use().uniform("projection", projection);
    }

    void process_input();
    void update();
    void render() {
        for (auto&& tile : levels_[current_level_].tiles()) {
            renderer_.draw_sprite(tile.sprite(), tile.transform());
        }
    }

};
