#pragma once
#include <glbinding/gl/types.h>
#include <glbinding/gl/boolean.h>

/*
Exposes basic scalars used by OpenGL: GLint, GLenum, GLsizei, etc.
into the project namespace.

Do not expose glbinding specific stuff like Masks.
Do not expose types that appear rarely, only ones that are (or could be) commonly used.

Extend as needed.
*/
namespace learn {

using gl::GLenum;
using gl::GLboolean;
using gl::GLbitfield;
using gl::GLvoid;
using gl::GLint, gl::GLuint;
using gl::GLint64, gl::GLuint64;
using gl::GLsizei;
using gl::GLfloat, gl::GLdouble;
using gl::GLchar;
using gl::GLintptr, gl::GLsizeiptr;


} // namespace learn
