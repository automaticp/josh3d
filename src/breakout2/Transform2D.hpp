#pragma once
#include "Transform.hpp"
#include <glm/glm.hpp>


struct Transform2D {
    glm::vec2 position;
    glm::vec2 scale;
    float angle_rad;

    learn::MTransform mtransform() const noexcept {
        return learn::Transform()
            .scale({ scale, 1.f })
            .rotate(angle_rad, { 0.f, 0.f, 1.f })
            .translate({ position, 0.f });
    }
};
