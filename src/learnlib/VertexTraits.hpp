#pragma once
#include <array>
#include <glbinding/gl/gl.h>



namespace learn {



struct AttributeParams {
    gl::GLuint      index;
    gl::GLint       size;
    gl::GLenum      type;
    gl::GLboolean   normalized;
    gl::GLsizei     stride_bytes;
    gl::GLint64     offset_bytes;
};


template<typename V>
struct VertexTraits {
    static_assert(sizeof(V) == 0, "Custom vertex class V must have a VertexTraits<V> specialization. There's no default that makes sense.");
    constexpr static std::array<AttributeParams, 0> aparams;
};



} // namespace learn
