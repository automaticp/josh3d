#pragma once
#include "GLScalars.hpp"


namespace josh {

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


} // namespace josh
