#pragma once
#include "Ball.hpp"
#include "Circle2D.hpp"
#include "Rect2D.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>

// true if collides, false otherwise
inline bool check_collision(Rect2D lhs, Rect2D rhs) noexcept {
    return glm::all(
        glm::lessThan(glm::abs(lhs.center - rhs.center), (lhs.size + rhs.size) / 2.f));
}


inline bool check_collision(Rect2D rect, Circle2D circle) noexcept {
    // Offset from the rectangle center, by calculation.
    auto closest_point_offset = glm::clamp(
        circle.center - rect.center, -rect.half_size(), rect.half_size());
    return glm::length((closest_point_offset + rect.center) - circle.center) <
           circle.radius;
}


inline bool check_collision(Circle2D lhs, Circle2D rhs) noexcept {
    return glm::length(lhs.center - rhs.center) < (lhs.radius + rhs.radius);
}


enum class RectXCollisionType { none, left, right };
enum class RectYCollisionType { none, top, bottom };

struct InnerRectCollisionInfo {
    RectXCollisionType x_collision{ RectXCollisionType::none };
    RectYCollisionType y_collision{ RectYCollisionType::none };
    glm::vec2 overshoot{ 0.f, 0.f };

    bool did_collide() const noexcept {
        return x_collision != RectXCollisionType::none ||
               y_collision != RectYCollisionType::none;
    }
};


InnerRectCollisionInfo inner_ball_on_rect_collision(
    const Rect2D& rect, const Ball& ball, glm::vec2 dxdy);




// The side of the rectangle
// that the ball collided with,
// or 'none' if no collision occured.
enum class RectCollisionType : size_t { none = 0, left, right, top, bottom };

// Get the side of the rectangle the collision occured from
RectCollisionType outer_collision_direction(glm::vec2 difference_vector);




struct OuterRectCollisionInfo {
    RectCollisionType type{ RectCollisionType::none };
    glm::vec2 difference{ 0.f, 0.f };

    bool did_collide() const noexcept {
        return type != RectCollisionType::none;
    }
};

OuterRectCollisionInfo outer_ball_on_rect_collision(
    const Rect2D& rect, const Ball& ball, glm::vec2 dxdy);

// Apply the corrections to the ball's velocity and
// dxdy of the step according to the outer collision parameters.
//
// Check whether collision actually occured before calling this.
void apply_outer_collision_correction(
    Ball& ball, glm::vec2& dxdy, const OuterRectCollisionInfo& collision);
