#pragma once
#include "GLScalars.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <chrono>


// Various queries of the OpenGL state.
// Specific queries are added here on a per-need basis.
namespace josh::glapi::queries {


// Wraps `glGetInteger64v` with `pname = GL_TIMESTAMP`.
//
// THIS IS NOT AN ASYNCHRONOUS QUERY.
//
// The current time of the GL may be queried by calling GetIntegerv or GetInteger64v
// with the symbolic constant `GL_TIMESTAMP`. This will return the GL time
// after all previous commands have reached the GL server but have not yet necessarily executed.
// By using a combination of this synchronous get command and the
// asynchronous timestamp query object target, applications can measure the latency
// between when commands reach the GL server and when they are realized in the framebuffer.
inline std::chrono::nanoseconds current_time() noexcept {
    GLint64 current_time;
    gl::glGetInteger64v(gl::GL_TIMESTAMP, &current_time);
    return std::chrono::nanoseconds{ current_time };
}




} // namespace josh::glapi::limits

