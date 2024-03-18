#pragma once
#include "GLScalars.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


// Various implementation limits of the OpenGL API.
// Specific limit queries are added here on a per-need basis.
namespace josh::glapi::limits {


// TODO: Replace this maybe?
// This construct is common enough that it could be part of utils.
// Also passing this as an array of floats is UB, great.
// But glm does it anyway, great.
struct RangeF {
    float min, max;
};




// Returns a pair of values indicating the range of widths supported for aliased lines. See glLineWidth.
inline auto aliased_line_width_range() noexcept -> RangeF {
    float r[2]; gl::glGetFloatv(gl::GL_ALIASED_LINE_WIDTH_RANGE, r);
    return { r[0], r[1] };
}


inline auto max_color_attachments() noexcept -> GLuint {
    GLint result;
    gl::glGetIntegerv(gl::GL_MAX_COLOR_ATTACHMENTS, &result);
    return GLuint(result);
}


} // namespace josh::glapi::limits
