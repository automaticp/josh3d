#include "Collisions.hpp"

#include <range/v3/all.hpp>

void apply_outer_collision_correction(
    Ball& ball, glm::vec2& dxdy, const OuterRectCollisionInfo& collision) {

    const auto& [dir, diff_vector] = collision;

    if ( dir == RectCollisionType::left || dir == RectCollisionType::right ) {

        ball.velocity().x *= -1.f;

        const float penetration = ball.radius() - glm::abs(diff_vector.x);

        if ( dir == RectCollisionType::left ) {
            dxdy.x -= 2.f * penetration;
        } else {
            dxdy.x += 2.f * penetration;
        }
    } else /* vertical */ {

        ball.velocity().y *= -1.f;

        const float penetration = ball.radius() - glm::abs(diff_vector.y);

        if ( dir == RectCollisionType::top ) {
            dxdy.y += 2.f * penetration;
        } else {
            dxdy.y -= 2.f * penetration;
        }
    }
}


OuterRectCollisionInfo outer_ball_on_rect_collision(
    const Rect2D& rect, const Ball& ball, glm::vec2 dxdy) {

    const glm::vec2 new_ball_center = ball.center() + dxdy;

    const glm::vec2 center_difference = new_ball_center - rect.center;

    const glm::vec2 clamped =
        glm::clamp(center_difference, -rect.half_size(), rect.half_size());

    const glm::vec2 closest = rect.center + clamped;
    const glm::vec2 difference = closest - new_ball_center;

    bool overlaps = glm::length(difference) < ball.radius();

    if ( overlaps ) {
        return {
            .type = outer_collision_direction(difference),
            .difference = difference
        };
    } else {
        return {};
    }
}


RectCollisionType outer_collision_direction(glm::vec2 difference_vector) {
    constexpr std::array<glm::vec2, 4> compass{ {
        { -1.f, 0.f }, // left
        { 1.f, 0.f },  // right
        { 0.f, 1.f },  // top
        { 0.f, -1.f }  // bottom
    } };

    auto me = ranges::max_element(compass, {},
        [&](const glm::vec2& dir) { return glm::dot(dir, -difference_vector); });
    size_t elem_idx = ranges::distance(ranges::begin(compass), me);
    return RectCollisionType{ elem_idx + 1 };
}


InnerRectCollisionInfo inner_ball_on_rect_collision(
    const Rect2D& rect, const Ball& ball, glm::vec2 dxdy) {

    const glm::vec2 new_pos = ball.center() + dxdy;
    const glm::vec2 move_direction = glm::sign(dxdy);
    const glm::vec2 new_edge_pos = new_pos + move_direction * ball.radius();

    InnerRectCollisionInfo info{};
    auto& [x_collision, y_collision, overshoot] = info;

    if ( new_edge_pos.x > rect.bound_right() ) {

        overshoot.x = new_edge_pos.x - rect.bound_right();
        x_collision = RectXCollisionType::right;

    } else if ( new_edge_pos.x < rect.bound_left() ) {

        overshoot.x = new_edge_pos.x - rect.bound_left();
        x_collision = RectXCollisionType::left;
    }


    if ( new_edge_pos.y > rect.bound_top() ) {

        overshoot.y = new_edge_pos.y - rect.bound_top();
        y_collision = RectYCollisionType::top;

    } else if ( new_edge_pos.y < rect.bound_bottom() ) {

        overshoot.y = new_edge_pos.y - rect.bound_bottom();
        y_collision = RectYCollisionType::bottom;
    }

    return info;
}
