#pragma once
#include "GLScalars.hpp"


namespace learn {

/*
Attribute specification pack for attaching
VBOs to Vertex Arrays.
*/
struct AttributeParams {
    GLuint      index;
    GLint       size;
    GLenum      type;
    GLboolean   normalized;
    GLsizei     stride_bytes;
    GLint64     offset_bytes;
};


} // namespace learn
