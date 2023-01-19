#pragma once
#include "Globals.hpp"
#include "Rect2D.hpp"
#include "Sprite.hpp"
#include "Transform.hpp"

#include <glm/glm.hpp>




class Paddle {
private:
    Rect2D box_;
    glm::vec2 velocity_{ 0.f, 0.f };
    Sprite sprite_;


public:
    explicit Paddle(Rect2D bounding_box)
        : box_{ bounding_box },
        sprite_{
            learn::globals::texture_handle_pool.load("src/breakout/sprites/paddle.png")
        }
    {}

    Rect2D& box() noexcept { return box_; }
    const Rect2D& box() const noexcept { return box_; }
    glm::vec2& center() noexcept { return box_.center; }
    const glm::vec2& center() const noexcept { return box_.center; }
    glm::vec2& size() noexcept { return box_.size; }
    const glm::vec2& size() const noexcept { return box_.size; }
    glm::vec2& velocity() noexcept { return velocity_; }
    const glm::vec2& velocity() const noexcept { return velocity_; }

    learn::Transform get_transform() noexcept { return box_.get_transform(); }

    const Sprite& sprite() const noexcept { return sprite_; }

};
