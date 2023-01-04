#pragma once
#include "Particle2D.hpp"
#include "Sprite.hpp"

#include <algorithm>
#include <memory>
#include <vector>
#include <random>

class Particle2DGenerator {
private:
    std::unique_ptr<std::mt19937> rng_{ std::make_unique<std::mt19937>() };
    std::vector<Particle2D> particles_;
    std::vector<Particle2D>::iterator last_dead_particle_{ particles_.begin() };
    Sprite sprite_;
    std::normal_distribution<float> lifetime_dist_;
    glm::vec2 origin_;
    glm::vec2 offset_variance_;
    glm::vec4 color_decay_;


public:
    Particle2DGenerator(
        size_t max_n_particles, Sprite sprite, std::normal_distribution<float> lifetime_dist,
        glm::vec2 origin, glm::vec2 offset_variance, glm::vec4 color_decay
    )
        : particles_( max_n_particles )
        , sprite_{ std::move(sprite) }
        , lifetime_dist_{ lifetime_dist }
        , origin_{ origin }
        , offset_variance_{ offset_variance }
        , color_decay_{ color_decay }
    {}

    void update(float dt, glm::vec2 reset_velocity) noexcept {
        // auto& p = *find_next_dead_particle();
        for (auto& p : particles_) {
            p.lifetime -= dt;
            if (p.lifetime <= 0.f) {
                reset_particle(p, reset_velocity);
            }
            p.position += p.velocity * dt;
            p.color -= color_decay_ * dt;
            p.velocity *= 0.85f;
        }
    }

    void set_origin(glm::vec2 new_origin) noexcept {
        origin_ = new_origin;
    }

    const auto& particles() const noexcept { return particles_; }
    const Sprite& sprite() const noexcept { return sprite_; }

private:
    std::vector<Particle2D>::iterator find_next_dead_particle() noexcept {

        auto is_dead = [](Particle2D& p) { return p.lifetime <= 0.f; };

        // Search from last dead to the end
        auto it = std::find_if(last_dead_particle_, particles_.end(), is_dead);

        if (it != particles_.end()) {
            return last_dead_particle_ = it;
        } else {
            // If not found, search from the beginning
            it = std::find_if(particles_.begin(), last_dead_particle_, is_dead);

            if (it != last_dead_particle_) {
                return last_dead_particle_ = it;
            } else {
                // If no dead particles at all, return first
                return last_dead_particle_ = particles_.begin();
            }
        }
    }


    void reset_particle(Particle2D& p, glm::vec2 reset_velocity) {
        p.position = origin_ + get_random_offset();
        p.scale = { 7.f, 7.f };
        p.lifetime = get_random_lifetime();
        p.color = glm::vec4{ 0.8f, 0.8f, 0.4f, 1.0f };
        p.velocity = reset_velocity;
    }

    glm::vec2 get_random_offset() noexcept {
        std::normal_distribution<float> dist_x{ 0.f, offset_variance_.x };
        std::normal_distribution<float> dist_y{ 0.f, offset_variance_.y };
        return { dist_x(*rng_), dist_y(*rng_) };
    }

    float get_random_lifetime() noexcept {
        return lifetime_dist_(*rng_);
    }
};
