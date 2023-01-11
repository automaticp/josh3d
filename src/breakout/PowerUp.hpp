#pragma once
#include "Globals.hpp"
#include "Rect2D.hpp"
#include "Sprite.hpp"
#include <glm/glm.hpp>
#include <cstddef>
#include <stdexcept>


enum class PowerUpType : size_t {
    none = 0,
    speed = 1,
    sticky = 2,
    pass_through = 3,
    pad_size_up = 4,
    confuse = 5,
    chaos = 6
};



class PowerUp {
private:
    PowerUpType type_;
    Rect2D box_;
    glm::vec2 velocity_;
    Sprite sprite_;
    bool is_alive_{ true };

public:
    PowerUp(PowerUpType type, Rect2D bounding_box, glm::vec2 velocity)
        : type_{ type }
        , box_{ bounding_box }
        , velocity_{ velocity }
        , sprite_{ get_sprite_for_type(type_) }
    {}


    learn::Transform get_transform() const noexcept { return box_.get_transform(); }

    const Sprite& sprite() const noexcept { return sprite_; }

    const Rect2D& box() const noexcept { return box_; }
    Rect2D& box() noexcept { return box_; }

    glm::vec2& velocity() noexcept { return velocity_; }
    const glm::vec2& velocity() const noexcept { return velocity_; }

    PowerUpType type() const noexcept { return type_; }

    void destroy() noexcept { is_alive_ = false; }
    bool is_alive() const noexcept { return is_alive_; }


private:
    static Sprite get_sprite_for_type(PowerUpType type) {
        const glm::vec4 color_good{ 0.f, 1.f, 0.f, 1.f };
        const glm::vec4 color_bad { 1.f, 0.f, 0.f, 1.f };

        auto& pool = learn::globals::texture_handle_pool;

        switch (type) {
            case PowerUpType::speed:
                return {
                    pool.load("src/breakout/sprites/powerup_speed.png"),
                    color_good
                };
            case PowerUpType::sticky:
                return {
                    pool.load("src/breakout/sprites/powerup_sticky.png"),
                    color_good
                };
            case PowerUpType::pass_through:
                return {
                    pool.load("src/breakout/sprites/powerup_passthrough.png"),
                    color_good
                };
            case PowerUpType::pad_size_up:
                return {
                    pool.load("src/breakout/sprites/powerup_increase.png"),
                    color_good
                };
            case PowerUpType::confuse:
                return {
                    pool.load("src/breakout/sprites/powerup_confuse.png"),
                    color_bad
                };
            case PowerUpType::chaos:
                return {
                    pool.load("src/breakout/sprites/powerup_chaos.png"),
                    color_bad
                };
            default:
                throw std::runtime_error("Invalid powerup type. Does not have a sprite.");
        }
    }

};

