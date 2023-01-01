#pragma once

#include "Transform.hpp"
#include <glm/glm.hpp>


struct Circle2D {
    glm::vec2 center;
    float radius;

    learn::Transform get_transform() const noexcept {
        return learn::Transform()
            .translate({ center.x, center.y, 0.f })
            .scale({ radius, radius, 1.f });
    }
};
