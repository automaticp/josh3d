#pragma once

#include "Transform.hpp"
#include <glm/glm.hpp>

struct Particle2D {
    glm::vec2 position;
    glm::vec2 scale;
    glm::vec2 velocity;
    glm::vec4 color;
    float lifetime{ 0.f };

    learn::Transform get_transform() const noexcept {
        return learn::Transform()
            .translate({ position, 0.f })
            .scale({scale, 1.f});
    }
};
