#pragma once
#include <glbinding/gl/types.h>   // IWYU pragma: export
#include <glbinding/gl/boolean.h> // IWYU pragma: export

/*
Exposes basic scalars used by OpenGL: GLint, GLenum, GLsizei, etc.
into the project namespace.

Do not expose glbinding specific stuff like Masks.
Do not expose types that appear rarely, only ones that are (or could be) commonly used.

Extend as needed.
*/
namespace josh {

using gl::GLenum;
using gl::GLboolean;
using gl::GLbitfield;
using gl::GLvoid;
using gl::GLint, gl::GLuint;
using gl::GLint64, gl::GLuint64;
using gl::GLsizei;
using gl::GLfloat, gl::GLdouble;
using gl::GLchar, gl::GLbyte, gl::GLubyte;
using gl::GLshort, gl::GLushort;
using gl::GLintptr, gl::GLsizeiptr;


} // namespace josh
