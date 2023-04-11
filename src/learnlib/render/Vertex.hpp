#pragma once
#include <array>
#include <vector>
#include <span>
#include <cstddef>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "VertexTraits.hpp"


namespace learn {



struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
};


template<>
struct VertexTraits<Vertex> {
    constexpr static std::array<AttributeParams, 3> aparams{{
        { 0u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex), offsetof(Vertex, position) },
        { 1u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal) },
        { 2u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex_uv) }
    }};
};







} // namespace learn
