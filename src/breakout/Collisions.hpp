#pragma once
#include "Circle2D.hpp"
#include "Rect2D.hpp"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>

// true if collides, false otherwise
inline bool check_collision(Rect2D lhs, Rect2D rhs) noexcept {
    return glm::all(glm::lessThan(lhs.center - rhs.center, (lhs.size + rhs.size) / 2.f));
}


inline bool check_collision(Rect2D rect, Circle2D circle) noexcept {
    // Offset from the rectangle center, by calculation.
    auto closest_point_offset =
        glm::clamp(circle.center - rect.center, -rect.half_size(), rect.half_size());
    return glm::length((closest_point_offset + rect.center) - circle.center) < circle.radius;
}


inline bool check_collision(Circle2D lhs, Circle2D rhs) noexcept {
    return glm::length(lhs.center - rhs.center) < (lhs.radius + rhs.radius);
}
