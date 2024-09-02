#pragma once
#include <glm/glm.hpp>


namespace josh {


struct AABB {
    glm::vec3 lbb; // Left-Bottom-Back
    glm::vec3 rtf; // Right-Top-Front

    auto extents()  const noexcept -> glm::vec3 { return rtf - lbb;       }
    auto midpoint() const noexcept -> glm::vec3 { return (rtf + lbb) / 2; }

};


struct LocalAABB {
    glm::vec3 lbb; // Left-Bottom-Back
    glm::vec3 rtf; // Right-Top-Front

    auto extents()  const noexcept -> glm::vec3 { return rtf - lbb;       }
    auto midpoint() const noexcept -> glm::vec3 { return (rtf + lbb) / 2; }
};


} // namespace josh
