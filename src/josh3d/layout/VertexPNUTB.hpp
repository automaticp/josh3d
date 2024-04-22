#pragma once
#include <glm/glm.hpp>


namespace josh {


struct VertexPNUTB {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};


} // namespace josh
