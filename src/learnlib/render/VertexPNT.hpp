#pragma once
#include "VertexConcept.hpp"
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <span>
#include <vector>


namespace learn {



struct VertexPNT {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;


    using attributes_type = AttributeParams[3];
    static const attributes_type& get_attributes() noexcept { return aparams; }
    static const attributes_type aparams;
};




inline const VertexPNT::attributes_type VertexPNT::aparams{
    { 0u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNT), offsetof(VertexPNT, position) },
    { 1u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNT), offsetof(VertexPNT, normal) },
    { 2u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNT), offsetof(VertexPNT, tex_uv) }
};


static_assert(vertex<VertexPNT>);





} // namespace learn
