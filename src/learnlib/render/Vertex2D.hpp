#pragma once
#include "VertexTraits.hpp"

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <array>
#include <cstddef>


namespace learn {


struct Vertex2D {
    glm::vec2 position;
    glm::vec2 tex_uv;
};


template<>
struct VertexTraits<Vertex2D> {
    constexpr static std::array<AttributeParams, 2> aparams{{
        {0u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex2D), offsetof(Vertex2D, position)},
        {1u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex2D), offsetof(Vertex2D, tex_uv)}
    }};
};

} // namespace learn
