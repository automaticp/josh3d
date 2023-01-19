#pragma once

#include "Transform.hpp"
#include <glm/glm.hpp>
#include <glm/vector_relational.hpp>
#include <glm/ext.hpp>

// Simple class for working with square bounds.


struct Rect2D {
    glm::vec2 center;
    glm::vec2 size;

    Rect2D(glm::vec2 center, glm::vec2 size_xy)
        : center{ center }, size{ size_xy } {}

    // { x0, y0, x1, y1 }
    Rect2D(glm::vec4 verts)
        : center{
            midpoint({ verts[0], verts[2] }),
            midpoint({ verts[1], verts[3] })
        },
        size{
            glm::abs(verts[0] - verts[2]),
            glm::abs(verts[1] - verts[3]),
        }
    {}

    float bound_left()   const noexcept { return center.x - size.x / 2.0f; }
    float bound_right()  const noexcept { return center.x + size.x / 2.0f; }
    float bound_bottom() const noexcept { return center.y - size.y / 2.0f; }
    float bound_top()    const noexcept { return center.y + size.y / 2.0f; }

    float width()  const noexcept { return glm::abs(size.x); }
    float height() const noexcept { return glm::abs(size.y); }

    glm::vec2 half_size() const noexcept { return size / 2.0f; }

    bool contains(glm::vec2 point) const noexcept {
        glm::vec2 dxdy = glm::abs(point - center);
        return glm::all(glm::lessThanEqual(dxdy, half_size()));
    }

    learn::Transform get_transform() const noexcept {
        return learn::Transform()
            .translate({ center.x, center.y, 0.f })
            .scale({ size.x, size.y, 1.f });
    }

private:
    static float midpoint(glm::vec2 segment) noexcept {
        return 0.5f * (segment[0] + segment[1]);
    }

};

