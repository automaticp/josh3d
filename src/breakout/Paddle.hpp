#pragma once
#include "Globals.hpp"
#include "Sprite.hpp"
#include "Transform.hpp"




class Paddle {
private:
    learn::Transform transform_;
    Sprite sprite_;

public:
    explicit Paddle(const learn::Transform& transform)
        : transform_{ transform },
        sprite_{
            learn::globals::texture_handle_pool.load("src/breakout/sprites/paddle.png")
        }
    {}

    learn::Transform& transform() noexcept { return transform_; }
    const learn::Transform& transform() const noexcept { return transform_; }
    const Sprite& sprite() const noexcept { return sprite_; }


};
