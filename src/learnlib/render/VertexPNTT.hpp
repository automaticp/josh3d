#pragma once
#include "VertexConcept.hpp"
#include "AttributeParams.hpp"
#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <cstddef>


namespace learn {





struct VertexPNTT {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
    glm::vec3 tangent;


    using attributes_type = AttributeParams[4];
    static const attributes_type& get_attributes() noexcept { return aparams; }
    static const attributes_type aparams;
};



inline const VertexPNTT::attributes_type VertexPNTT::aparams{
    { 0u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTT), offsetof(VertexPNTT, position) },
    { 1u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTT), offsetof(VertexPNTT, normal) },
    { 2u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTT), offsetof(VertexPNTT, tex_uv) },
    { 3u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTT), offsetof(VertexPNTT, tangent) }
};


static_assert(vertex<VertexPNTT>);



} // namespace learn
