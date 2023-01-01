#pragma once
#include "Circle2D.hpp"
#include "Globals.hpp"
#include "Sprite.hpp"


class Ball {
private:
    Circle2D circle_;
    Sprite sprite_;

public:
    explicit Ball(Circle2D circle)
        : circle_{ circle },
        sprite_{
            learn::globals::texture_handle_pool.load("src/breakout/sprites/awesomeface.png")
        }
    {}

    Circle2D& circle() noexcept { return circle_; }
    const Circle2D& circle() const noexcept { return circle_; }
    glm::vec2& center() noexcept { return circle_.center; }
    const glm::vec2& center() const noexcept { return circle_.center; }
    float& radius() noexcept { return circle_.radius; }
    float radius() const noexcept { return circle_.radius; }

    learn::Transform get_transform() const noexcept { return circle_.get_transform(); }

    const Sprite& sprite() const noexcept { return sprite_; }
};
