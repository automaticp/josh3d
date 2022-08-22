#pragma once
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <array>
#include <cstddef>


namespace learn {

using namespace gl;


struct AttributeParams {
    GLuint      index;
    GLint       size;
    GLenum      type;
    GLboolean   normalized;
    GLsizei     stride_bytes;
    GLint64     offset_bytes;
};


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
};


template<typename V>
struct VertexTraits {
    static_assert(sizeof(V) == 0, "Custom vertex class V must have a VertexTraits<V> specialization. There's no default that makes sense.");
    constexpr static std::array<AttributeParams, 0> aparams;
};


template<>
struct VertexTraits<Vertex> {
    constexpr static std::array<AttributeParams, 3> aparams{{
        { 0u, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position) },
        { 1u, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal) },
        { 2u, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex_uv) }
    }};
};



} // namespace learn
