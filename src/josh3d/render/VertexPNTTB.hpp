#pragma once
#include "VertexConcept.hpp"
#include "AttributeParams.hpp"
#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <cstddef>


namespace josh {





struct VertexPNTTB {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;


    using attributes_type = AttributeParams[5];
    static const attributes_type& get_attributes() noexcept { return aparams; }
    static const attributes_type aparams;
};



inline const VertexPNTTB::attributes_type VertexPNTTB::aparams{
    { 0u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, position) },
    { 1u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, normal) },
    { 2u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, tex_uv) },
    { 3u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, tangent) },
    { 4u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, bitangent) }
};


static_assert(vertex<VertexPNTTB>);



} // namespace josh
