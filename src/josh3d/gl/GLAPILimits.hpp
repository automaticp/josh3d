#pragma once
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLScalars.hpp"
#include "detail/GLAPIGet.hpp"
#include <glbinding/gl/enum.h>


/*
Various implementation limits of the OpenGL API.
Specific limit queries are added here on a per-need basis.

TODO: Why not just have a couple of enums?
*/
namespace josh::glapi {

enum class LimitI : GLuint
{
    MaxVertTextureUnits     = GLuint(gl::GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS),
    MaxTescTextureUnits     = GLuint(gl::GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS),
    MaxTeseTextureUnits     = GLuint(gl::GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS),
    MaxGeomTextureUnits     = GLuint(gl::GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS),
    MaxFragTextureUnits     = GLuint(gl::GL_MAX_TEXTURE_IMAGE_UNITS),
    MaxCompTextureUnits     = GLuint(gl::GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS),
    MaxTextureUnitsCombined = GLuint(gl::GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS),
};


namespace limits {

inline auto get(LimitI limit)
    -> GLint
{
    return detail::get_integer(enum_cast<GLenum>(limit));
}

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



} // namespace limits
} // namespace josh::glapi
