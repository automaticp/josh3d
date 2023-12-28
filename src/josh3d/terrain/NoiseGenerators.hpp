#pragma once
#include <memory>
#include <random>


namespace josh {


class WhiteNoiseGenerator {
private:
    std::unique_ptr<std::mt19937> bitsgen_;

public:
    WhiteNoiseGenerator()
        : WhiteNoiseGenerator(std::random_device{}())
    {}

    WhiteNoiseGenerator(size_t seed)
        : bitsgen_{ std::make_unique<std::mt19937>(seed) }
    {}

    WhiteNoiseGenerator(std::seed_seq seed_sequence)
        : bitsgen_{ std::make_unique<std::mt19937>(seed_sequence) }
    {}

    float operator()() const noexcept {
        std::uniform_real_distribution<float> dist{};
        return dist(*bitsgen_);
    }
};


} // namespace josh
