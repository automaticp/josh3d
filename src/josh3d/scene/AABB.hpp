#pragma once
#include "Size.hpp"
#include <glm/glm.hpp>


namespace josh {


struct AABB {
    glm::vec3 lbb; // Left-Bottom-Back
    glm::vec3 rtf; // Right-Top-Front

    Extent3F extent() const noexcept;
};


struct LocalAABB {
    glm::vec3 lbb; // Left-Bottom-Back
    glm::vec3 rtf; // Right-Top-Front

    Extent3F extent() const noexcept;
};


} // namespace josh
