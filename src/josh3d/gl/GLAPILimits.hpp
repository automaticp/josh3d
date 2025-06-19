#pragma once
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLScalars.hpp"
#include "detail/GLAPIGet.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/types.h>


/*
Various implementation limits of the OpenGL API.
Specific limit queries are added here on a per-need basis.
*/
namespace josh {

enum class LimitB : GLuint
{
    PrimitiveRestartForPatchesSupported = GLuint(gl::GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitB,
    PrimitiveRestartForPatchesSupported);

enum class LimitI : GLuint
{
    MaxVertexAttributes      = GLuint(gl::GL_MAX_VERTEX_ATTRIBS),
    MaxVertexAttribBindings  = GLuint(gl::GL_MAX_VERTEX_ATTRIB_BINDINGS),
    MaxVertTextureUnits      = GLuint(gl::GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS),
    MaxTescTextureUnits      = GLuint(gl::GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS),
    MaxTeseTextureUnits      = GLuint(gl::GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS),
    MaxGeomTextureUnits      = GLuint(gl::GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS),
    MaxFragTextureUnits      = GLuint(gl::GL_MAX_TEXTURE_IMAGE_UNITS),
    MaxCompTextureUnits      = GLuint(gl::GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS),
    MaxTextureUnitsCombined  = GLuint(gl::GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS),
    // _max_compute_workgroups_x
    // _max_compute_workgroups_y
    // _max_compute_workgroups_z
    // _max_compute_workgroup_invocations
    // _max_compute_workgroup_size_x
    // _max_compute_workgroup_size_y
    // _max_compute_workgroup_size_z
    MaxDrawBuffers           = GLuint(gl::GL_MAX_DRAW_BUFFERS),
    MaxDualSourceDrawBuffers = GLuint(gl::GL_MAX_DUAL_SOURCE_DRAW_BUFFERS),
    MaxColorAttachments      = GLuint(gl::GL_MAX_COLOR_ATTACHMENTS),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitI,
    MaxVertexAttributes,
    MaxVertexAttribBindings,
    MaxVertTextureUnits,
    MaxTescTextureUnits,
    MaxTeseTextureUnits,
    MaxGeomTextureUnits,
    MaxFragTextureUnits,
    MaxCompTextureUnits,
    MaxTextureUnitsCombined,
    MaxDrawBuffers,
    MaxDualSourceDrawBuffers,
    MaxColorAttachments);

enum class LimitF : GLuint
{
    PointSizeGranularity       = GLuint(gl::GL_POINT_SIZE_GRANULARITY),
    SmoothPointSizeGranularity = GLuint(gl::GL_SMOOTH_POINT_SIZE_GRANULARITY),
    LineWidthGranularity       = GLuint(gl::GL_LINE_WIDTH_GRANULARITY),
    SmoothLineWidthGranularity = GLuint(gl::GL_SMOOTH_LINE_WIDTH_GRANULARITY),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitF,
    PointSizeGranularity,  // SmoothPointSizeGranularity
    LineWidthGranularity); // SmoothLineWidthGranularity

enum class LimitRF : GLuint
{
    PointSizeRange        = GLuint(gl::GL_POINT_SIZE_RANGE),
    AliasedPointSizeRange = GLuint(gl::GL_ALIASED_POINT_SIZE_RANGE),
    LineWidthRange        = GLuint(gl::GL_LINE_WIDTH_RANGE),
    AliasedLineWidthRange = GLuint(gl::GL_ALIASED_LINE_WIDTH_RANGE),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitRF,
    PointSizeRange,
    AliasedPointSizeRange,
    LineWidthRange,
    AliasedLineWidthRange);


namespace glapi {

inline auto get_limit(LimitB limit)
    -> GLboolean
{
    return detail::get_boolean(enum_cast<GLenum>(limit));
}

inline auto get_limit(LimitI limit)
    -> GLint
{
    return detail::get_integer(enum_cast<GLenum>(limit));
}

inline auto get_limit(LimitRF limit)
    -> RangeF
{
    auto [min, max] = detail::get_floatv<2>(enum_cast<GLenum>(limit));
    return { min, max };
}

} // namespace glapi

namespace limits {

// TODO: Deprecate.
// Returns a pair of values indicating the range of widths supported for aliased lines. See glLineWidth.
[[deprecated]]
inline auto aliased_line_width_range() noexcept
    -> RangeF
{
    return glapi::get_limit(LimitRF::AliasedLineWidthRange);
}

[[deprecated]]
inline auto max_color_attachments() noexcept
    -> GLuint
{
    GLint result;
    gl::glGetIntegerv(gl::GL_MAX_COLOR_ATTACHMENTS, &result);
    return GLuint(result);
}



} // namespace limits
} // namespace josh::glapi
