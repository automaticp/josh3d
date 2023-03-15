#include "PowerUp.hpp"
#include "SpriteRenderSystem.hpp"
#include "Transform2D.hpp"
#include "GlobalsGL.hpp"
#include "PhysicsSystem.hpp"
#include <utility>


static Sprite get_powerup_sprite(PowerUpType);

void make_powerup(entt::registry& registry, PhysicsSystem& physics, PowerUpType type, glm::vec2 pos) {

    constexpr glm::vec2 pup_scale{ 100.f, 20.f };
    constexpr glm::vec2 pup_velocity{ 0.f, -150.f };

    auto pup = registry.create();
    registry.emplace<Transform2D>(pup, pos, pup_scale, 0.f);
    registry.emplace<Sprite>(pup, get_powerup_sprite(type));

    PhysicsComponent phys{ physics.create_powerup(pup, pos, pup_scale) };
    phys.set_velocity(pup_velocity);

    registry.emplace<PhysicsComponent>(pup, std::move(phys));

    registry.emplace<PowerUpComponent>(pup, type);

}


static Sprite get_powerup_sprite(PowerUpType type) {

    const glm::vec4 color_good{ 0.f, 1.f, 0.f, 1.f };
    const glm::vec4 color_bad { 1.f, 0.f, 0.f, 1.f };

    auto& pool = learn::globals::texture_handle_pool;

    switch (type) {
        case PowerUpType::speed:
            return {
                pool.load("src/breakout2/sprites/powerup_speed.png"),
                ZDepth::foreground,
                color_good
            };
        case PowerUpType::sticky:
            return {
                pool.load("src/breakout2/sprites/powerup_sticky.png"),
                ZDepth::foreground,
                color_good
            };
        case PowerUpType::pass_through:
            return {
                pool.load("src/breakout2/sprites/powerup_passthrough.png"),
                ZDepth::foreground,
                color_good
            };
        case PowerUpType::pad_size_up:
            return {
                pool.load("src/breakout2/sprites/powerup_increase.png"),
                ZDepth::foreground,
                color_good
            };
        case PowerUpType::confuse:
            return {
                pool.load("src/breakout2/sprites/powerup_confuse.png"),
                ZDepth::foreground,
                color_bad
            };
        case PowerUpType::chaos:
            return {
                pool.load("src/breakout2/sprites/powerup_chaos.png"),
                ZDepth::foreground,
                color_bad
            };
        default:
            throw std::runtime_error("Invalid powerup type. Does not have a sprite.");
    }
}



