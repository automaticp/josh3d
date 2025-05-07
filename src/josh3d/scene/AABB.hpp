#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>


namespace josh {


/*
AABB in arbitrary space.

As a component, it represents world-space AABB.
*/
struct AABB {
    glm::vec3 lbb; // Left-Bottom-Back
    glm::vec3 rtf; // Right-Top-Front

    auto extents()  const noexcept -> glm::vec3 { return rtf - lbb;       }
    auto midpoint() const noexcept -> glm::vec3 { return (rtf + lbb) / 2; }

    auto transformed(const glm::mat4& world_mat) const noexcept -> AABB;
};


inline auto AABB::transformed(const glm::mat4& world_mat) const noexcept
    -> AABB
{
    using glm::vec3, glm::vec4;

    // Pick total 3+1 vertices around one corner (lbb).
    const vec3 rbb = vec3(rtf.x, lbb.y, lbb.z);
    const vec3 ltb = vec3(lbb.x, rtf.y, lbb.z);
    const vec3 lbf = vec3(lbb.x, lbb.y, rtf.z);

    // Transform them to world. They are no longer axis-aligned.
    const vec3 lbb_tfd = world_mat * vec4(lbb, 1.f);
    const vec3 rbb_tfd = world_mat * vec4(rbb, 1.f);
    const vec3 ltb_tfd = world_mat * vec4(ltb, 1.f);
    const vec3 lbf_tfd = world_mat * vec4(lbf, 1.f);

    // Midpoint should still be correct after transformation.
    const vec3 new_midpoint = world_mat * vec4(midpoint(), 1.f);

    // Largest distance to midpoint along each axis wins.
    const vec3 lbb_dm = glm::abs(lbb_tfd - new_midpoint);
    const vec3 rbb_dm = glm::abs(rbb_tfd - new_midpoint);
    const vec3 ltb_dm = glm::abs(ltb_tfd - new_midpoint);
    const vec3 lbf_dm = glm::abs(lbf_tfd - new_midpoint);

    const float max_dm_x = std::max({ lbb_dm.x, rbb_dm.x, ltb_dm.x, lbf_dm.x });
    const float max_dm_y = std::max({ lbb_dm.y, rbb_dm.y, ltb_dm.y, lbf_dm.y });
    const float max_dm_z = std::max({ lbb_dm.z, rbb_dm.z, ltb_dm.z, lbf_dm.z });

    const vec3 max_dm = vec3(max_dm_x, max_dm_y, max_dm_z);

    const vec3 new_lbb = new_midpoint - max_dm;
    const vec3 new_rtf = new_midpoint + max_dm;

    return { new_lbb, new_rtf };
}


/*
AABB in local space of the object.
*/
struct LocalAABB : AABB {};


} // namespace josh
