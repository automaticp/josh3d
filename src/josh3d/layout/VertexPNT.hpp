#pragma once
#include <glm/glm.hpp>


namespace josh {


struct VertexPNT {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
};


} // namespace josh
