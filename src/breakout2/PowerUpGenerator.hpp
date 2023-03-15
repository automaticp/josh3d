#pragma once
#include "PhysicsSystem.hpp"
#include "PowerUp.hpp"
#include <entt/entt.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <numeric>
#include <random>


class PowerUpGenerator {
private:
    std::unique_ptr<std::mt19937> rng_{
        std::make_unique<std::mt19937>(
            std::random_device{}()
        )
    };

public:
    void try_generate_random_at(entt::registry& reg, PhysicsSystem& phys, glm::vec2 position) {

        const auto type = PowerUpType{ get_random_distributed_index() };

        if (type != PowerUpType::none) {
            make_powerup(reg, phys, type, position);
        }

    }

private:
    static constexpr std::array<float, 7> base_chance_weights{
    //  none, speed, sticky, pass_through, pad_size_up, confuse, chaos
        60.f, 1.f,   1.f,    1.f,          1.f,         3.f,     3.f
    };

    static constexpr std::array<float, 7> normalize_chance_weights(const std::array<float, 7>& weights) {
        const float sum = std::reduce(weights.begin(), weights.end());
        auto result = base_chance_weights;
        for (auto& elem : result) { elem /= sum; }
        return result;
    }

    inline static const std::array<float, 7> base_probabilities{
        normalize_chance_weights(base_chance_weights)
    };


    size_t get_random_distributed_index() {
        std::uniform_real_distribution<float> dist;
        float gamma{ dist(*rng_) };

        size_t i{ 0 };
        for (; i < base_probabilities.size(); ++i) {
            gamma -= base_probabilities[i];
            if (gamma < 0.f) { return i; }
        }
        // Fallback for FP error
        return base_probabilities.size() - 1;
    }

};
