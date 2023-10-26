#pragma once
#include <glm/glm.hpp>


namespace josh::components {


/*
Simple pivot-centered sphere that fully encloses the object.
*/
struct BoundingSphere {
    float radius;

    float scaled_radius(const glm::vec3& scale) const noexcept {
        return glm::max(glm::max(scale.x, scale.y), scale.z) * radius;
    }
};


} // namespace josh::components
