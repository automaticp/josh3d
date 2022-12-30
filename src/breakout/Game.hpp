#pragma once
#include "All.hpp"
#include "FrameTimer.hpp"
#include "GLObjectPools.hpp"
#include "Sprite.hpp"

#include <vector>


enum class GameState {
    active, win, menu
};


class Game {
private:
    GameState state_;
    SpriteRenderer& renderer_;
    const learn::FrameTimer& frame_timer_;

    std::vector<Sprite> sprites_;

public:
    Game(learn::FrameTimer& frame_timer, SpriteRenderer& sprite_renderer)
        : frame_timer_( frame_timer ), renderer_{ sprite_renderer } {}

    Game(SpriteRenderer& sprite_renderer)
        : Game(learn::global_frame_timer, sprite_renderer) {}

    void init() {
        sprites_.clear();

        // Gotta do something about the pools, though
        sprites_.emplace_back(
            *learn::global_texture_handle_pool.load("src/breakout/sprites/awesomeface.png"),
            learn::Transform().scale({ 200.f, 200.f, 1.0f })
            // color
        );

        sprites_.emplace_back(
            *learn::global_texture_handle_pool.load("src/breakout/sprites/awesomeface.png"),
            learn::Transform().scale({ 200.f, 200.f, 1.0f }).translate({1.f, 1.f, 0.f}),
            glm::vec3{ 0.5f, 1.0f, 0.3f }
        );


        // Figure out this line later
        glm::mat4 projection{
            glm::ortho(0.0f, 600.f, 800.f, 0.0f, -1.0f, 1.0f)
        };

        renderer_.shader().use().uniform("projection", projection);
    }

    void process_input();
    void update();
    void render() {
        for (auto&& sprite : sprites_) {
            renderer_.draw_sprite(sprite);
        }
    }

};
