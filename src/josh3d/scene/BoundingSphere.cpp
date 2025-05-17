#include "BoundingSphere.hpp"
#include "Math.hpp"
#include <glm/ext.hpp>


namespace josh {


auto BoundingSphere::transformed(const glm::mat4& world_mat) const noexcept
    -> BoundingSphere
{
    const mat3 W2L_mat3 = mat3(world_mat);

    // Standard basis of the sphere will get deformed by the transformation.
    const float new_x_length2 = glm::length2(W2L_mat3 * X);
    const float new_y_length2 = glm::length2(W2L_mat3 * Y);
    const float new_z_length2 = glm::length2(W2L_mat3 * Z);

    // The largest new basis vector is the radius multiplier.
    const float scale_factor = glm::sqrt(glm::max(glm::max(new_x_length2, new_y_length2), new_z_length2));
    const float new_radius   = radius * scale_factor;

    // Translation is as usual.
    const vec3 new_position = world_mat * vec4{ position, 1.f };

    return { new_position, new_radius };
}


} // namespace josh
