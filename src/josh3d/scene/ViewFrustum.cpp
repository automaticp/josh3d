#include "ViewFrustum.hpp"


namespace josh {


auto ViewFrustumAsQuads::make_local_z_symmetric(
    float near_width, float near_height,
    float far_width,  float far_height,
    float z_near,     float z_far) noexcept
        -> ViewFrustumAsQuads
{
    const float nhw = near_width  / 2.f;
    const float nhh = near_height / 2.f;
    Quad near{
        glm::vec3{ -nhw,  nhh, z_near },
        glm::vec3{ -nhw, -nhh, z_near },
        glm::vec3{  nhw, -nhh, z_near },
        glm::vec3{  nhw,  nhh, z_near },
    };
    const float fhw = far_width  / 2.f;
    const float fhh = far_height / 2.f;
    Quad far{
        glm::vec3{ -fhw,  fhh, z_far },
        glm::vec3{ -fhw, -fhh, z_far },
        glm::vec3{  fhw, -fhh, z_far },
        glm::vec3{  fhw,  fhh, z_far },
    };
    return ViewFrustumAsQuads{ near, far };
}


auto ViewFrustumAsQuads::make_local_perspective(
    float fovy_rad, float aspect_ratio,
    float z_near,   float z_far) noexcept
        -> ViewFrustumAsQuads
{
    using std::tan, std::atan;

    const float vfov = fovy_rad;
    const float hfov = 2.f * atan((aspect_ratio) * tan(vfov / 2.f));

    const float height_far = 2.f * tan(vfov / 2.f) * z_far;
    const float width_far  = 2.f * tan(hfov / 2.f) * z_far;

    const float height_near = height_far * (z_near / z_far);
    const float width_near  = width_far  * (z_near / z_far);

    return ViewFrustumAsQuads::make_local_z_symmetric(
        width_near, height_near, width_far, height_far, z_near, z_far
    );
}


auto ViewFrustumAsQuads::transformed(const glm::mat4& world_mat) const noexcept
    -> ViewFrustumAsQuads
{
    return { near_.transformed(world_mat), far_.transformed(world_mat) };
}


auto ViewFrustumAsPlanes::make_local_perspective(
    float fovy_rad, float aspect_ratio,
    float z_near,   float z_far) noexcept
        -> ViewFrustumAsPlanes
{
    // RH (X: right, Y: up, Z: back) coordinate system.

    Plane near{ { 0.f, 0.f,  1.f }, z_near };
    Plane far { { 0.f, 0.f, -1.f }, z_far  };

    using std::sin, std::cos, std::tan, std::atan;

    // aspect == w / h == tan(hfov/2) / tan(vfov/2)
    const float vfov = fovy_rad;
    const float hfov = 2.f * atan((aspect_ratio) * tan(vfov / 2.f));

    Plane right{ { cos(hfov / 2.f),   0.f,             sin(hfov / 2.f) } };
    Plane top  { { 0.f,               cos(vfov / 2.f), sin(vfov / 2.f) } };

    Plane left { { -right.normal.x,   0.f,             right.normal.z  } };
    Plane btm  { { 0.f,               -top.normal.y,   top.normal.z    } };

    return { near, far, left, right, top, btm };
}


auto ViewFrustumAsPlanes::make_local_orthographic(
    float left_side,   float right_side,
    float bottom_side, float top_side,
    float z_near,      float z_far) noexcept
        -> ViewFrustumAsPlanes
{
    const Plane near { {  0.f,  0.f,  1.f }, -z_near      };
    const Plane far  { {  0.f,  0.f, -1.f },  z_far       };
    const Plane right{ {  1.f,  0.f,  0.f },  right_side  };
    const Plane left { { -1.f,  0.f,  0.f }, -left_side   };
    const Plane top  { {  0.f,  1.f,  0.f },  top_side    };
    const Plane btm  { {  0.f, -1.f,  0.f }, -bottom_side };
    return { near, far, left, right, top, btm };
}


auto ViewFrustumAsPlanes::make_local_orthographic(
    float width,  float height,
    float z_near, float z_far) noexcept
        -> ViewFrustumAsPlanes
{
    const float hw = width  / 2.f;
    const float hh = height / 2.f;
    return make_local_orthographic(-hw, hw, -hh, hh, z_near, z_far);
}


auto ViewFrustumAsPlanes::transformed(const glm::mat4& world_mat) const noexcept
    -> ViewFrustumAsPlanes
{
    return {
        near_.transformed(world_mat), far_   .transformed(world_mat),
        left_.transformed(world_mat), right_ .transformed(world_mat),
        top_ .transformed(world_mat), bottom_.transformed(world_mat),
    };
}




} // namespace josh
