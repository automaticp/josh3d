#pragma once

#include "Transform.hpp"
#include <glm/glm.hpp>


struct Circle2D {
    glm::vec2 center;
    float radius;

    learn::MTransform get_transform() const noexcept {
        return learn::MTransform()
            .translate({ center.x, center.y, 0.f })
            .scale({ radius * 2.f, radius * 2.f, 1.f });
    }
};
