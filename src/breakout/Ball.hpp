#pragma once
#include "Circle2D.hpp"
#include "GlobalsGL.hpp"
#include "Sprite.hpp"


class Ball {
private:
    Circle2D circle_;
    glm::vec2 velocity_{ 0.f, 0.f };
    Sprite sprite_;
    bool is_stuck_{ true };

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
    glm::vec2& velocity() noexcept { return velocity_; }
    const glm::vec2& velocity() const noexcept { return velocity_; }

    learn::MTransform get_transform() const noexcept { return circle_.get_transform(); }

    const Sprite& sprite() const noexcept { return sprite_; }

    void make_unstuck() noexcept { is_stuck_ = false; }
    void make_stuck() noexcept {
        velocity_ = { 0.f, 0.f };
        is_stuck_ = true;
    }
    bool is_stuck() const noexcept { return is_stuck_; }
};
