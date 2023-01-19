#pragma once
#include "PowerUp.hpp"
#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <random>
#include <range/v3/all.hpp>
#include <numeric>
#include <vector>

class PowerUpGenerator {
private:
    std::vector<PowerUp> powerups_;

    std::unique_ptr<std::mt19937> rng_{
        std::make_unique<std::mt19937>(
            std::random_device{}()
        )
    };

public:
    PowerUpGenerator() = default;

    std::span<PowerUp> powerups() noexcept { return powerups_; }
    std::span<const PowerUp> powerups() const noexcept { return powerups_; }

    PowerUp& generate_at(glm::vec2 position, PowerUpType type) {
        powerups_.emplace_back(
            PowerUp(
                type,
                Rect2D{ position, { 50.f, 12.f }},
                { 0.f, -200.f }
            )
        );
        return powerups_.back();
    }

    PowerUp* try_generate_random_at(glm::vec2 position) {
        auto type = PowerUpType{ get_distributed_index() };
        if (type == PowerUpType::none) {
            return nullptr;
        }

        powerups_.emplace_back(
            PowerUp(
                type,
                Rect2D{ position, { 50.f, 12.f }},
                { 0.f, -200.f }
            )
        );

        return &powerups_.back();
    }

    void remove_destroyed() {
        powerups_.erase(
            std::remove_if(
                powerups_.begin(), powerups_.end(),
                [](const PowerUp& pup) {
                    return !pup.is_alive();
                }
            ),
            powerups_.end()
        );
    }

private:
    size_t get_distributed_index() {
        std::uniform_real_distribution<float> dist;
        auto gamma = dist(*rng_);

        size_t i{ 0 };
        for (; i < chances.size() || gamma < 0.0f; ++i) {
            gamma -= chances[i];
            if (gamma < 0.f) { return i; }
        }
        // Fallback for fp error
        return chances.size() - 1;
    }

    constexpr static std::array<float, 7> get_normalized_probabilities() {
        std::array<float, 7> absolute{
        //  none
            60.f,
        //  speed, sticky, pass_through, pad_size_up
            1.f,   1.f,    1.f,          1.f,
        //  confuse, chaos
            3.f,     3.f
        };

        auto sum = std::reduce(absolute.begin(), absolute.end());

        for (auto& elem : absolute) {
            elem /= sum;
        }

        return absolute;
    }

    inline const static std::array<float, 7> chances{
        get_normalized_probabilities()
    };
};
