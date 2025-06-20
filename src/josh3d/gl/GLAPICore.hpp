#pragma once
#include "DecayToRaw.hpp"
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPITargets.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "Region.hpp"
#include "GLPixelPackTraits.hpp"
#include "detail/GLAPIGet.hpp"
#include <chrono>
#include <cassert>
#include <span>


namespace josh {


/*
SECTION: Queries.
*/
namespace glapi {

/*
Wraps `glGetInteger64v` with `pname = GL_TIMESTAMP`.

THIS IS NOT AN ASYNCHRONOUS QUERY.

The current time of the GL may be queried by calling GetIntegerv or GetInteger64v
with the symbolic constant `GL_TIMESTAMP`. This will return the GL time
after all previous commands have reached the GL server but have not yet necessarily executed.
By using a combination of this synchronous get command and the
asynchronous timestamp query object target, applications can measure the latency
between when commands reach the GL server and when they are realized in the framebuffer.
*/
inline auto get_current_time()
    -> std::chrono::nanoseconds
{
    return std::chrono::nanoseconds(detail::get_integer64(gl::GL_TIMESTAMP));
}

} // namespace glapi


/*
SECTION: Draw and Dispatch.

TODO: TransformFeedbacks? Does anyone use them?
*/
enum class Primitive : GLuint
{
    Points                 = GLuint(gl::GL_POINTS),
    Lines                  = GLuint(gl::GL_LINES),
    LinesAdjacency         = GLuint(gl::GL_LINES_ADJACENCY),
    LineStrip              = GLuint(gl::GL_LINE_STRIP),
    LineStripAdjacency     = GLuint(gl::GL_LINE_STRIP_ADJACENCY),
    LineLoop               = GLuint(gl::GL_LINE_LOOP),
    Triangles              = GLuint(gl::GL_TRIANGLES),
    TrianglesAdjacency     = GLuint(gl::GL_TRIANGLES_ADJACENCY),
    TriangleStrip          = GLuint(gl::GL_TRIANGLE_STRIP),
    TriangleStripAdjacency = GLuint(gl::GL_TRIANGLE_STRIP_ADJACENCY),
    TriangleFan            = GLuint(gl::GL_TRIANGLE_FAN),
    Patches                = GLuint(gl::GL_PATCHES),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Primitive,
    Points,
    Lines,
    LinesAdjacency,
    LineStrip,
    LineStripAdjacency,
    LineLoop,
    Triangles,
    TrianglesAdjacency,
    TriangleStrip,
    TriangleStripAdjacency,
    TriangleFan,
    Patches);

enum class ElementType : GLuint
{
    UByte  = GLuint(gl::GL_UNSIGNED_BYTE),
    UShort = GLuint(gl::GL_UNSIGNED_SHORT),
    UInt   = GLuint(gl::GL_UNSIGNED_INT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(ElementType, UByte, UShort, UInt);

namespace glapi {

inline void draw_arrays(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    GLint                               vertex_offset,
    GLsizei                             vertex_count)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    gl::glDrawArrays(enum_cast<GLenum>(primitive), vertex_offset, vertex_count);
}

inline void draw_elements(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    ElementType                         type,
    GLsizeiptr                          element_offset_bytes,
    GLsizei                             element_count)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    gl::glDrawElements(
        enum_cast<GLenum>(primitive),
        element_count, enum_cast<GLenum>(type),
        (const void*)element_offset_bytes); // NOLINT
}

inline void multidraw_arrays(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    // TODO: Arguments are easily confused.
    std::span<const GLint>              vertex_offsets,
    std::span<const GLsizei>            vertex_counts)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    assert(vertex_offsets.size() == vertex_counts.size());
    gl::glMultiDrawArrays(
        enum_cast<GLenum>(primitive),
        vertex_offsets.data(),
        vertex_counts.data(),
        GLsizei(vertex_counts.size()));
}

inline void multidraw_elements(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    ElementType                         type,
    // TODO: Arguments are easily confused.
    std::span<const GLsizeiptr>         element_offsets_bytes,
    std::span<const GLsizei>            element_counts)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    assert(element_offsets_bytes.size() == element_counts.size());
    using offset_t = const void*;
    gl::glMultiDrawElements(
        enum_cast<GLenum>(primitive),
        element_counts.data(),
        enum_cast<GLenum>(type),
        (const offset_t*)element_offsets_bytes.data(),
        GLsizei(element_counts.size()));
}

inline void draw_elements_basevertex(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    ElementType                         type,
    // TODO: Arguments are easily confused.
    GLsizeiptr                          element_offset_bytes,
    GLsizei                             element_count,
    GLint                               element_basevert)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    using offset_t = const void*;
    gl::glDrawElementsBaseVertex(
        enum_cast<GLenum>(primitive),
        element_count,
        enum_cast<GLenum>(type),
        (offset_t)element_offset_bytes, // NOLINT
        element_basevert);
}

inline void multidraw_elements_basevertex(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    ElementType                         type,
    // TODO: Arguments are easily confused.
    std::span<const GLsizeiptr>         element_offsets_bytes,
    std::span<const GLsizei>            element_counts,
    std::span<const GLint>              element_baseverts)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    assert(element_offsets_bytes.size() == element_counts.size());
    assert(element_offsets_bytes.size() == element_baseverts.size());
    using offset_t = const void*;
    gl::glMultiDrawElementsBaseVertex(
        enum_cast<GLenum>(primitive),
        element_counts.data(),
        enum_cast<GLenum>(type),
        (const offset_t*)element_offsets_bytes.data(),
        GLsizei(element_counts.size()),
        element_baseverts.data());
}

inline void _draw_arrays_instanced()
{
    // gl::glDrawArraysInstanced()
}

inline void _draw_arrays_instanced_baseinstance()
{
    // gl::glDrawArraysInstancedBaseInstance()
}

inline void draw_elements_instanced(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    GLsizei                             instance_count,
    Primitive                           primitive,
    ElementType                         type,
    GLsizeiptr                          element_offset_bytes,
    GLsizei                             element_count)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    gl::glDrawElementsInstanced(
        enum_cast<GLenum>(primitive),
        element_count,
        enum_cast<GLenum>(type),
        (const void*)element_offset_bytes, // NOLINT
        instance_count);
}

inline void _draw_elements_instanced_baseinstance()
{
    // gl::glDrawElementsInstancedBaseInstance()
}

inline void _draw_elements_instanced_basevertex_baseinstance()
{
    // gl::glDrawElementsInstancedBaseVertexBaseInstance()
}

inline void _draw_elements_range()
{
    // gl::glDrawRangeElements()
}

inline void _draw_elements_range_basevertex()
{
    // gl::glDrawRangeElementsBaseVertex()
}

namespace limits {
// TODO: What's a more correct name?
inline auto _recommended_max_num_vertices_per_draw()  {
    return detail::get_integer(gl::GL_MAX_ELEMENTS_VERTICES);
}
inline auto _recommended_max_num_indices_per_draw()  {
    return detail::get_integer(gl::GL_MAX_ELEMENTS_VERTICES);
}
} // namespace limits

inline void dispatch_compute(
    BindToken<Binding::Program> bound_program [[maybe_unused]],
    GLuint                      num_groups_x,
    GLuint                      num_groups_y,
    GLuint                      num_groups_z)
{
    assert(bound_program.id() == get_bound_id(Binding::Program));
    gl::glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}

/*
"[4.6, 10.3.11] Arguments to the indirect commands DrawArraysIndirect,
DrawElementsIndirect, MultiDrawArraysIndirect, and MultiDrawElementsIndirect
(see section 10.4), and to DispatchComputeIndirect (see section 19) are sourced from
the buffer object currently bound to the corresponding indirect buffer target (see
table 10.7), using the command’s indirect parameter as an offset into the buffer object
in the same fashion as described in section 10.3.9. Buffer objects are created
and/or bound to a target as described in section 6.1. Initially zero is bound to each
target.

Arguments are stored in buffer objects as structures (for *Draw*Indirect) or
arrays (for DispatchComputeIndirect) of tightly packed 32-bit integers."
*/

/*
An INVALID_VALUE error is generated if indirect is not a multiple of the
size, in basic machine units, of uint.
*/
inline void _draw_elements_indirect()
{
    // gl::glDrawElementsIndirect()
}

inline void multidraw_elements_indirect(
    BindToken<Binding::VertexArray>        bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>            bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer>    bound_draw_framebuffer [[maybe_unused]],
    BindToken<Binding::DrawIndirectBuffer> bound_indirect_buffer  [[maybe_unused]],
    Primitive                              primitive,
    ElementType                            type,
    GLsizei                                draw_count,
    GLsizeiptr                             indirect_buffer_offset_bytes,
    GLsizei                                indirect_buffer_stride_bytes)
{
    assert(bound_program.id()          == get_bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == get_bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == get_bound_id(Binding::VertexArray));
    assert(bound_indirect_buffer.id()  == get_bound_id(Binding::DrawIndirectBuffer));
    using offset_t = const void*;
    gl::glMultiDrawElementsIndirect(
        enum_cast<GLenum>(primitive),
        enum_cast<GLenum>(type),
        (offset_t)indirect_buffer_offset_bytes, // NOLINT
        draw_count,
        indirect_buffer_stride_bytes);
}

inline void _multidraw_elements_indirect_count()
{
    // gl::glMultiDrawElementsIndirectCount()
}

} // namespace glapi


/*
SECTION: Capabilities.
*/
enum class Capability : GLuint
{
    SeamlessCubemaps           = GLuint(gl::GL_TEXTURE_CUBE_MAP_SEAMLESS),
    PrimitiveRestart           = GLuint(gl::GL_PRIMITIVE_RESTART),
    PrimitiveRestartFixedIndex = GLuint(gl::GL_PRIMITIVE_RESTART_FIXED_INDEX),
    DiscardRasterizer          = GLuint(gl::GL_RASTERIZER_DISCARD),
    ScissorTesting             = GLuint(gl::GL_SCISSOR_TEST),
    StencilTesting             = GLuint(gl::GL_STENCIL_TEST),
    DepthTesting               = GLuint(gl::GL_DEPTH_TEST),
    Blending                   = GLuint(gl::GL_BLEND),
    Multisampling              = GLuint(gl::GL_MULTISAMPLE),
    PerSampleShading           = GLuint(gl::GL_SAMPLE_SHADING),
    SRGBConversion             = GLuint(gl::GL_FRAMEBUFFER_SRGB),
    Dithering                  = GLuint(gl::GL_DITHER),
    ColorLogicalOp             = GLuint(gl::GL_COLOR_LOGIC_OP),
    ProgramSpecifiedPointSize  = GLuint(gl::GL_PROGRAM_POINT_SIZE),
    AntialiasedPoints          = GLuint(gl::GL_POINT_SMOOTH),   // NOTE: Deprecated.
    AntialiasedLines           = GLuint(gl::GL_LINE_SMOOTH),    // NOTE: Deprecated.
    AntialiasedPolygons        = GLuint(gl::GL_POLYGON_SMOOTH), // NOTE: Deprecated.
    FaceCulling                = GLuint(gl::GL_CULL_FACE),
    PolygonOffsetPoint         = GLuint(gl::GL_POLYGON_OFFSET_POINT),
    PolygonOffsetLine          = GLuint(gl::GL_POLYGON_OFFSET_LINE),
    PolygonOffsetFill          = GLuint(gl::GL_POLYGON_OFFSET_FILL),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Capability,
    SeamlessCubemaps,
    PrimitiveRestart,
    PrimitiveRestartFixedIndex,
    DiscardRasterizer,
    ScissorTesting,
    StencilTesting,
    DepthTesting,
    Blending,
    Multisampling,
    PerSampleShading,
    SRGBConversion,
    Dithering,
    ColorLogicalOp,
    ProgramSpecifiedPointSize,
    AntialiasedPoints,
    AntialiasedLines,
    AntialiasedPolygons,
    FaceCulling,
    PolygonOffsetPoint,
    PolygonOffsetLine,
    PolygonOffsetFill);

enum class CapabilityIndexed : GLuint
{
    ScissorTest = GLuint(gl::GL_SCISSOR_TEST),
    Blending    = GLuint(gl::GL_BLEND),
};
JOSH3D_DEFINE_ENUM_EXTRAS(CapabilityIndexed, ScissorTest, Blending);

namespace glapi {

inline void enable(Capability cap)
{
    gl::glEnable(enum_cast<GLenum>(cap));
}

inline void disable(Capability cap)
{
    gl::glDisable(enum_cast<GLenum>(cap));
}

inline auto is_enabled(Capability cap)
    -> bool
{
    return bool(gl::glIsEnabled(enum_cast<GLenum>(cap)));
}

inline void enable(CapabilityIndexed cap, GLuint index)
{
    gl::glEnablei(enum_cast<GLenum>(cap), index);
}

inline void disable(CapabilityIndexed cap, GLuint index)
{
    gl::glDisablei(enum_cast<GLenum>(cap), index);
}

inline auto is_enabled(CapabilityIndexed cap, GLuint index)
    -> bool
{
    return bool(gl::glIsEnabledi(enum_cast<GLenum>(cap), index));
}

} // namespace glapi


/*
SECTION: Flush and Finish [2.3.3]
*/
namespace glapi {

inline void finish()
{
    gl::glFinish();
}

inline void flush()
{
    gl::glFlush();
}

} // namespace glapi


/*
SECTION: Shader Memory Access Synchronization [7.13.2]
*/
enum class BarrierMask : GLuint
{
    VertexAttribArrayBit  = GLuint(gl::GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT),
    ElementArrayBit       = GLuint(gl::GL_ELEMENT_ARRAY_BARRIER_BIT),
    UniformBit            = GLuint(gl::GL_UNIFORM_BARRIER_BIT),
    TextureFetchBit       = GLuint(gl::GL_TEXTURE_FETCH_BARRIER_BIT),
    ShaderImageAccessBit  = GLuint(gl::GL_SHADER_IMAGE_ACCESS_BARRIER_BIT),
    CommandBit            = GLuint(gl::GL_COMMAND_BARRIER_BIT),
    PixelBufferBit        = GLuint(gl::GL_PIXEL_BUFFER_BARRIER_BIT),
    TextureUpdateBit      = GLuint(gl::GL_TEXTURE_UPDATE_BARRIER_BIT),
    BufferUpdateBit       = GLuint(gl::GL_BUFFER_UPDATE_BARRIER_BIT),
    ClientMappedBufferBit = GLuint(gl::GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT),
    QueryBufferBit        = GLuint(gl::GL_QUERY_BUFFER_BARRIER_BIT),
    FramebufferBit        = GLuint(gl::GL_FRAMEBUFFER_BARRIER_BIT),
    TransformFeedbackBit  = GLuint(gl::GL_TRANSFORM_FEEDBACK_BARRIER_BIT),
    AtomicCounterBit      = GLuint(gl::GL_ATOMIC_COUNTER_BARRIER_BIT),
    ShaderStorageBit      = GLuint(gl::GL_SHADER_STORAGE_BARRIER_BIT),
    AllBits               = GLuint(gl::GL_ALL_BARRIER_BITS),
};
JOSH3D_DEFINE_ENUM_BITSET_OPERATORS(BarrierMask);

namespace glapi {

inline void memory_barrier(BarrierMask barriers)
{
    gl::glMemoryBarrier(enum_cast<gl::MemoryBarrierMask>(barriers));
}

inline void texture_barrier()
{
    gl::glTextureBarrier();
}

} // namespace glapi


/*
SECTION: Multisampling.
*/
namespace glapi {

/*
The location in pixel space at which shading is performed for a given sample.
Pair of values in range [0, 1]. Pixel center is { 0.5, 0.5 }.
*/
inline auto get_sample_shading_location(GLuint sample_index)
    -> Offset2F
{
    float offsets[2];
    gl::glGetMultisamplefv(gl::GL_SAMPLE_POSITION, sample_index, offsets);
    return { offsets[0], offsets[1] };
}

/*
When both `Multisampling` and `SampleShading` are enabled, then
each fragment shader invocation recieves, at minimum, a number of samples equal to:
`max(ssr * samples, 1)`, where `ssr` is sample shading rate.

The value of `rate` is clamped to the range of [0, 1].
*/
inline void set_sample_shading_rate(GLfloat rate)
{
    gl::glMinSampleShading(rate);
}

inline auto get_sample_shading_rate()
    -> GLfloat
{
    return detail::get_float(gl::GL_MIN_SAMPLE_SHADING_VALUE);
}

} // namespace glapi


/*
SECTION: Point Rasterization Parameters.
*/
enum class PointSpriteCoordOrigin : GLuint
{
    LowerLeft = GLuint(gl::GL_LOWER_LEFT),
    UpperLeft = GLuint(gl::GL_UPPER_LEFT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(PointSpriteCoordOrigin, LowerLeft, UpperLeft);

namespace glapi {

inline void set_point_size(GLfloat size)
{
    gl::glPointSize(size);
}

inline auto get_point_size()
    -> GLfloat
{
    return detail::get_float(gl::GL_POINT_SIZE);
}

inline void set_point_fade_threshold(GLfloat threshold)
{
    gl::glPointParameterf(gl::GL_POINT_FADE_THRESHOLD_SIZE, threshold);
}

inline auto get_point_fade_threshold()
    -> GLfloat
{
    return detail::get_float(gl::GL_POINT_FADE_THRESHOLD_SIZE);
}

inline void set_point_sprite_coordinate_origin(PointSpriteCoordOrigin origin)
{
    gl::glPointParameteri(gl::GL_POINT_SPRITE_COORD_ORIGIN, enum_cast<GLenum>(origin));
}

inline auto get_point_sprite_coordinate_origin()
    -> PointSpriteCoordOrigin
{
    return detail::get_enum<PointSpriteCoordOrigin>(gl::GL_POINT_SPRITE_COORD_ORIGIN);
}

} // namespace glapi


/*
SECTION: Line Rasterization Parameters.
*/
namespace glapi {

inline void set_line_width(GLfloat width)
{
    gl::glLineWidth(width);
}

inline auto get_line_width()
    -> GLfloat
{
    return detail::get_float(gl::GL_LINE_WIDTH);
}

} // namespace glapi



/*
SECTION: Polygon Rasterization Parameters.

TODO: Clip Control
*/
enum class WindingOrder : GLuint
{
    CounterClockwise = GLuint(gl::GL_CCW),
    Clockwise        = GLuint(gl::GL_CW),
};
JOSH3D_DEFINE_ENUM_EXTRAS(WindingOrder, CounterClockwise, Clockwise);

enum class Faces : GLuint
{
    Front        = GLuint(gl::GL_FRONT),
    Back         = GLuint(gl::GL_BACK),
    FrontAndBack = GLuint(gl::GL_FRONT_AND_BACK),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Faces, Front, Back, FrontAndBack);

/*
"[14.6.4] Polygon antialiasing applies only to the FILL state of PolygonMode. For
POINT or LINE, point antialiasing or line segment antialiasing, respectively, apply."
*/
enum class PolygonRasterization : GLuint
{
    Point = GLuint(gl::GL_POINT),
    Line  = GLuint(gl::GL_LINE),
    Fill  = GLuint(gl::GL_FILL),
};
JOSH3D_DEFINE_ENUM_EXTRAS(PolygonRasterization, Point, Line, Fill);

namespace glapi {

inline void set_front_face_winding_order(WindingOrder order)
{
    gl::glFrontFace(enum_cast<GLenum>(order));
}

inline auto get_front_face_winding_order()
    -> WindingOrder
{
    return detail::get_enum<WindingOrder>(gl::GL_FRONT_FACE);
}

inline void set_face_culling_target(Faces culled_faces)
{
    gl::glCullFace(enum_cast<GLenum>(culled_faces));
}

inline auto get_face_culling_target()
    -> Faces
{
    return detail::get_enum<Faces>(gl::GL_CULL_FACE_MODE);
}

inline void set_polygon_rasterization_mode(PolygonRasterization mode)
{
    gl::glPolygonMode(gl::GL_FRONT_AND_BACK, enum_cast<GLenum>(mode));
}

inline auto get_polygon_rasterization_mode()
    -> PolygonRasterization
{
    return detail::get_enum<PolygonRasterization>(gl::GL_POLYGON_MODE);
}

inline auto set_polygon_offset_clamped(
    GLfloat slope_factor, GLfloat bias_scale, GLfloat bias_clamp)
{
    gl::glPolygonOffsetClamp(slope_factor, bias_scale, bias_clamp);
}

inline auto set_polygon_offset(
    GLfloat slope_factor, GLfloat bias_scale)
{
    gl::glPolygonOffset(slope_factor, bias_scale);
}

inline auto get_polygon_offset_slope_factor()
    -> GLfloat
{
    return detail::get_float(gl::GL_POLYGON_OFFSET_FACTOR);
}

inline auto get_polygon_offset_bias_scale()
    -> GLfloat
{
    return detail::get_float(gl::GL_POLYGON_OFFSET_UNITS);
}

inline auto get_polygon_offset_bias_clamp()
    -> GLfloat
{
    return detail::get_float(gl::GL_POLYGON_OFFSET_CLAMP);
}

} // namespace glapi


/*
SECTION: Viewport Control [???].
*/
namespace glapi {

inline void set_viewport(const Region2I& viewport_region)
{
    const auto [offset, extent] = viewport_region;
    gl::glViewport(offset.x, offset.y, extent.width, extent.height);
}

// TODO:
// See scissor test.
inline void _set_viewport_indexed() ;
inline void _set_viewports() ;

} // namespace glapi


/*
SECTION: Scissor Test [???].
*/
namespace glapi {

inline void set_scissor_region(const Region2I& region)
{
    const auto [offset, extent] = region;
    gl::glScissor(offset.x, offset.y, extent.width, extent.height);
}

inline void set_scissor_region_indexed(
    GLuint viewport_index, const Region2I& region)
{
    const auto [offset, extent] = region;
    gl::glScissorIndexed(viewport_index, offset.x, offset.y, extent.width, extent.height);
}

// TODO: Should be a version accepting the span<GLsizei>.
inline void set_scissor_regions(
    GLuint first_viewport_index, std::span<const Region2I> regions)
{
    gl::glScissorArrayv(
        first_viewport_index,
        GLsizei(regions.size()), // This isn't `size_bytes() / sizeof(int)`.
        // Shooo! UB go away.
        std::launder(reinterpret_cast<const GLint*>(regions.data()))
    );
}

// TODO: Does this work?
inline auto get_scissor_region()
    -> Region2I
{
    auto [x, y, w, h] = detail::get_integerv<4>(gl::GL_SCISSOR_BOX);
    return { { x, y }, { w, h } };
}

inline auto get_scissor_region_indexed(GLuint viewport_index)
    -> Region2I
{
    auto [x, y, w, h] = detail::get_integerv_indexed<4>(gl::GL_SCISSOR_BOX, viewport_index);
    return { { x, y }, { w, h } };
}

} // namespace glapi


/*
TODO SECTION: Multisample Fragment Operations [14.9.3].
*/


/*
TODO SECTION: Alpha To Coverage [17.3.1].
*/


/*
SECTION: Stencil Test [17.3.3].
*/
enum class StencilOp : GLuint
{
    Keep              = GLuint(gl::GL_KEEP),
    SetZero           = GLuint(gl::GL_ZERO),
    ReplaceWithRef    = GLuint(gl::GL_REPLACE),
    IncrementSaturate = GLuint(gl::GL_INCR),
    DecrementSaturate = GLuint(gl::GL_DECR),
    BitwiseInvert     = GLuint(gl::GL_INVERT),
    IncrementWrap     = GLuint(gl::GL_INCR_WRAP),
    DecrementWrap     = GLuint(gl::GL_DECR_WRAP),
};
JOSH3D_DEFINE_ENUM_EXTRAS(StencilOp,
    Keep,
    SetZero,
    ReplaceWithRef,
    IncrementSaturate,
    DecrementSaturate,
    BitwiseInvert,
    IncrementWrap,
    DecrementWrap);

namespace glapi {

/*
`stencil_test_pass = (ref & ref_mask) [op] stored_stencil_value`.
*/
inline void set_stencil_test_condition(
    Mask      ref_mask,
    GLint     ref,
    CompareOp op)
{
    gl::glStencilFunc(enum_cast<GLenum>(op), ref, ref_mask);
}

inline void set_stencil_test_condition(
    Face      face,
    Mask      ref_mask,
    GLint     ref,
    CompareOp op)
{
    gl::glStencilFuncSeparate(enum_cast<GLenum>(face), enum_cast<GLenum>(op), ref, ref_mask);
}

inline void set_stencil_test_operations(
    StencilOp on_stencil_fail,
    StencilOp on_stencil_pass_depth_fail,
    StencilOp on_stencil_pass_depth_pass)
{
    gl::glStencilOp(
        enum_cast<GLenum>(on_stencil_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_pass));
}

inline void set_stencil_test_operations(
    Face      face,
    StencilOp on_stencil_fail,
    StencilOp on_stencil_pass_depth_fail,
    StencilOp on_stencil_pass_depth_pass)
{
    gl::glStencilOpSeparate(
        enum_cast<GLenum>(face),
        enum_cast<GLenum>(on_stencil_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_pass));
}

inline auto get_stencil_test_condition_comapre_op(Face face)
    -> CompareOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_FUNC : gl::GL_STENCIL_BACK_FUNC };
    return detail::get_enum<CompareOp>(pname);
}

inline auto get_stencil_test_condition_ref(Face face)
    -> GLint
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_REF : gl::GL_STENCIL_BACK_REF };
    return detail::get_integer(pname);
}

inline auto get_stencil_test_condition_ref_mask(Face face)
    -> GLuint
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_VALUE_MASK : gl::GL_STENCIL_BACK_VALUE_MASK };
    return detail::get_integer(pname);
}

inline auto get_stencil_test_operation_on_stencil_fail(Face face)
    -> StencilOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_FAIL : gl::GL_STENCIL_BACK_FAIL };
    return detail::get_enum<StencilOp>(pname);
}

inline auto get_stencil_test_operation_on_stencil_pass_depth_fail(Face face)
    -> StencilOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_PASS_DEPTH_FAIL : gl::GL_STENCIL_BACK_PASS_DEPTH_FAIL };
    return detail::get_enum<StencilOp>(pname);
}

inline auto get_stencil_test_operation_on_stencil_pass_depth_pass(Face face)
    -> StencilOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_PASS_DEPTH_PASS : gl::GL_STENCIL_BACK_PASS_DEPTH_PASS };
    return detail::get_enum<StencilOp>(pname);
}

} // namespace glapi


/*
SECTION: Depth Buffer Test [17.3.4].

TODO: Depth Clamping [13.7], Depth Range [13.8]...
*/
namespace glapi {

/*
`depth_test_pass = incoming_depth [op] stored_depth`.
*/
inline void set_depth_test_condition(CompareOp op)
{
    gl::glDepthFunc(enum_cast<GLenum>(op));
}

inline auto get_depth_test_condition_compare_op()
    -> CompareOp
{
    return detail::get_enum<CompareOp>(gl::GL_DEPTH_FUNC);
}

} // namespace glapi


/*
SECTION: Blending [17.3.6].

TODO: Getters.
*/
enum class BlendEquation : GLuint
{
    FactorAdd             = GLuint(gl::GL_FUNC_ADD),
    FactorSubtract        = GLuint(gl::GL_FUNC_SUBTRACT),
    FactorReverseSubtract = GLuint(gl::GL_FUNC_REVERSE_SUBTRACT),
    Min                   = GLuint(gl::GL_MIN),
    Max                   = GLuint(gl::GL_MAX),
};
JOSH3D_DEFINE_ENUM_EXTRAS(BlendEquation,
    FactorAdd,
    FactorSubtract,
    FactorReverseSubtract,
    Min,
    Max);

/*
"Factor" is a replacement term for "Function" that is more accurate
for majority of cases and while less generic, a lot more clear on what it represents.
*/
enum class BlendFactor : GLuint
{
    Zero                  = GLuint(gl::GL_ZERO),
    One                   = GLuint(gl::GL_ONE),
    SrcColor              = GLuint(gl::GL_SRC_COLOR),
    OneMinusSrcColor      = GLuint(gl::GL_ONE_MINUS_SRC_COLOR),
    DstColor              = GLuint(gl::GL_DST_COLOR),
    OneMinusDstColor      = GLuint(gl::GL_ONE_MINUS_DST_COLOR),
    SrcAlpha              = GLuint(gl::GL_SRC_ALPHA),
    OneMinusSrcAlpha      = GLuint(gl::GL_ONE_MINUS_SRC_ALPHA),
    DstAlpha              = GLuint(gl::GL_DST_ALPHA),
    OneMinusDstAlpha      = GLuint(gl::GL_ONE_MINUS_DST_ALPHA),
    ConstantColor         = GLuint(gl::GL_CONSTANT_COLOR),
    OneMinusConstantColor = GLuint(gl::GL_ONE_MINUS_CONSTANT_COLOR),
    ConstantAlpha         = GLuint(gl::GL_CONSTANT_ALPHA),
    OneMinusConstantAlpha = GLuint(gl::GL_ONE_MINUS_CONSTANT_ALPHA),
    SrcAlphaSaturate      = GLuint(gl::GL_SRC_ALPHA_SATURATE),
    Src1Color             = GLuint(gl::GL_SRC1_COLOR),
    OneMinusSrc1Color     = GLuint(gl::GL_ONE_MINUS_SRC1_COLOR),
    Src1Alpha             = GLuint(gl::GL_SRC1_ALPHA),
    OneMinusSrc1Alpha     = GLuint(gl::GL_ONE_MINUS_SRC1_ALPHA),
};
JOSH3D_DEFINE_ENUM_EXTRAS(BlendFactor,
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha);

namespace glapi {

inline void set_blend_equation(BlendEquation equation)
{
    gl::glBlendEquation(enum_cast<GLenum>(equation));
}

inline void set_blend_equations(
    BlendEquation rgb_equation,
    BlendEquation alpha_equation)
{
    gl::glBlendEquationSeparate(
        enum_cast<GLenum>(rgb_equation),
        enum_cast<GLenum>(alpha_equation));
}

inline void set_blend_equation_indexed(
    GLuint        draw_buffer_index,
    BlendEquation equation)
{
    gl::glBlendEquationi(draw_buffer_index, enum_cast<GLenum>(equation));
}

inline void set_blend_equations_indexed(
    GLuint        draw_buffer_index,
    BlendEquation rgb_equation,
    BlendEquation alpha_equation)
{
    gl::glBlendEquationSeparatei(
        draw_buffer_index,
        enum_cast<GLenum>(rgb_equation),
        enum_cast<GLenum>(alpha_equation));
}

inline void set_blend_factors(
    BlendFactor src_factor,
    BlendFactor dst_factor)
{
    gl::glBlendFunc(
        enum_cast<GLenum>(src_factor),
        enum_cast<GLenum>(dst_factor));
}

// WARNING: The argument order is different from `glBlendFuncSeparate`.
inline void set_blend_factors(
    BlendFactor src_rgb_factor,
    BlendFactor src_alpha_factor,
    BlendFactor dst_rgb_factor,
    BlendFactor dst_alpha_factor)
{
    gl::glBlendFuncSeparate(
        enum_cast<GLenum>(src_rgb_factor),
        enum_cast<GLenum>(dst_rgb_factor),
        enum_cast<GLenum>(src_alpha_factor),
        enum_cast<GLenum>(dst_alpha_factor));
}

inline void set_blend_factors_indexed(
    GLuint      draw_buffer_index,
    BlendFactor src_factor,
    BlendFactor dst_factor)
{
    gl::glBlendFunci(
        draw_buffer_index,
        enum_cast<GLenum>(src_factor),
        enum_cast<GLenum>(dst_factor));
}

inline void set_blend_factors_indexed(
    GLuint      draw_buf_index,
    BlendFactor src_rgb_factor,
    BlendFactor src_alpha_factor,
    BlendFactor dst_rgb_factor,
    BlendFactor dst_alpha_factor)
{
    gl::glBlendFuncSeparatei(
        draw_buf_index,
        enum_cast<GLenum>(src_rgb_factor),
        enum_cast<GLenum>(dst_rgb_factor),
        enum_cast<GLenum>(src_alpha_factor),
        enum_cast<GLenum>(dst_alpha_factor));
}

inline void set_blend_constant_color(const RGBAF& color)
{
    const auto [r, g, b, a] = color;
    gl::glBlendColor(r, g, b, a);
}

} // namespace glapi


/*
SECTION: sRGB Conversion [17.3.7].
NOTE: Capability only.
*/


/*
SECTION: Dithering [17.3.8].
NOTE: Capability only.
*/


/*
SECTION: Logical Operation [17.3.9].
*/
enum class LogicOp : GLuint
{
    SetZero       = GLuint(gl::GL_CLEAR),
    SrcAndDst     = GLuint(gl::GL_AND),
    SrcAndNotDst  = GLuint(gl::GL_AND_REVERSE),
    Src           = GLuint(gl::GL_COPY),
    NotSrcAndDst  = GLuint(gl::GL_AND_INVERTED),
    Dst           = GLuint(gl::GL_NOOP),
    SrcXorDst     = GLuint(gl::GL_XOR),
    SrcOrDst      = GLuint(gl::GL_OR),
    Not_SrcOrDst  = GLuint(gl::GL_NOR),
    Not_SrcXorDst = GLuint(gl::GL_EQUIV),
    NotDst        = GLuint(gl::GL_INVERT),
    SrcOrNotDst   = GLuint(gl::GL_OR_REVERSE),
    NotSrc        = GLuint(gl::GL_COPY_INVERTED),
    NotSrcOrDst   = GLuint(gl::GL_OR_INVERTED),
    Not_SrcAndDst = GLuint(gl::GL_NAND),
    SetOne        = GLuint(gl::GL_SET),
};

namespace glapi {

inline void set_logical_operation(LogicOp operation)
{
    gl::glLogicOp(enum_cast<GLenum>(operation));
}

inline auto get_logical_operation()
    -> LogicOp
{
    return detail::get_enum<LogicOp>(gl::GL_LOGIC_OP_MODE);
}

} // namespace glapi


/*
SECTION: Fine Control of Buffer Updates (Write Masks) [17.4.2].
*/
enum class ColorMask
{
    Red   = (1 << 0),
    Green = (1 << 1),
    Blue  = (1 << 2),
    Alpha = (1 << 3),
};
JOSH3D_DEFINE_ENUM_BITSET_OPERATORS(ColorMask);
JOSH3D_DEFINE_ENUM_EXTRAS(ColorMask, Red, Green, Blue, Alpha);

namespace glapi {

inline void set_color_mask(
    GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha)
{
    gl::glColorMask(red, green, blue, alpha);
}

inline void set_color_mask(ColorMask mask_bits)
{
    set_color_mask(
        bool(mask_bits & ColorMask::Red),
        bool(mask_bits & ColorMask::Green),
        bool(mask_bits & ColorMask::Blue),
        bool(mask_bits & ColorMask::Alpha));
}

inline void set_color_mask_indexed(
    GLuint    draw_buffer_index,
    GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha)
{
    gl::glColorMaski(draw_buffer_index, red, green, blue, alpha);
}

inline void set_color_mask_indexed(
    GLuint    draw_buffer_index,
    ColorMask mask_bits)
{
    set_color_mask_indexed(
        draw_buffer_index,
        bool(mask_bits & ColorMask::Red),
        bool(mask_bits & ColorMask::Green),
        bool(mask_bits & ColorMask::Blue),
        bool(mask_bits & ColorMask::Alpha));
}

inline auto get_color_mask()
    -> ColorMask
{
    auto [r, g, b, a] = detail::get_booleanv<4>(gl::GL_COLOR_WRITEMASK);
    return
        (r ? ColorMask::Red   : ColorMask{}) |
        (g ? ColorMask::Green : ColorMask{}) |
        (b ? ColorMask::Blue  : ColorMask{}) |
        (a ? ColorMask::Alpha : ColorMask{});
}

inline auto get_color_mask_indexed(GLuint draw_buf_index)
    -> ColorMask
{
    auto [r, g, b, a] = detail::get_booleanv_indexed<4>(gl::GL_COLOR_WRITEMASK, draw_buf_index);
    return
        (r ? ColorMask::Red   : ColorMask{}) |
        (g ? ColorMask::Green : ColorMask{}) |
        (b ? ColorMask::Blue  : ColorMask{}) |
        (a ? ColorMask::Alpha : ColorMask{});
}

inline void set_depth_mask(GLboolean enabled_for_writing)
{
    gl::glDepthMask(enabled_for_writing);
}

inline auto get_depth_mask()
    -> GLboolean
{
    return detail::get_boolean(gl::GL_DEPTH_WRITEMASK);
}

// TODO: Shouldn't this use that Mask enum?
inline void set_stencil_mask(GLuint write_mask)
{
    gl::glStencilMask(write_mask);
}

inline void set_stencil_mask_per_face(Face face, GLuint write_mask)
{
    gl::glStencilMaskSeparate(enum_cast<GLenum>(face), write_mask);
}

inline auto get_stencil_mask(Face face)
    -> GLuint
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_WRITEMASK : gl::GL_STENCIL_BACK_WRITEMASK };
    return detail::get_integer(pname);
}

} // namespace glapi


/*
SECTION: Clearing the Buffers [17.4.3].
*/
namespace glapi {

inline void clear_color_buffer(
    BindToken<Binding::DrawFramebuffer> bound_fbo [[maybe_unused]],
    GLint                               buffer_index,
    const RGBAF&                        color_float)
{
    gl::glClearBufferfv(gl::GL_COLOR, buffer_index, &color_float.r);
}

inline void clear_color_buffer(
    BindToken<Binding::DrawFramebuffer> bound_fbo [[maybe_unused]],
    GLint                               buffer_index,
    const RGBAI&                        color_integer)
{
    gl::glClearBufferiv(gl::GL_COLOR, buffer_index, &color_integer.r);
}

inline void clear_color_buffer(
    BindToken<Binding::DrawFramebuffer> bound_fbo [[maybe_unused]],
    GLint                               buffer_index,
    const RGBAUI&                       color_uinteger)
{
    gl::glClearBufferuiv(gl::GL_COLOR, buffer_index, &color_uinteger.r);
}

inline void clear_depth_buffer(
    BindToken<Binding::DrawFramebuffer> bound_fbo [[maybe_unused]],
    GLfloat                             depth)
{
    gl::glClearBufferfv(gl::GL_DEPTH, 0, &depth);
}

inline void clear_stencil_buffer(
    BindToken<Binding::DrawFramebuffer> bound_fbo [[maybe_unused]],
    GLint                               stencil)
{
    gl::glClearBufferiv(gl::GL_STENCIL, 0, &stencil);
}

inline void clear_depth_stencil_buffer(
    BindToken<Binding::DrawFramebuffer> bound_fbo [[maybe_unused]],
    GLfloat                             depth,
    GLint                               stencil)
{
    gl::glClearBufferfi(gl::GL_DEPTH_STENCIL, 0, depth, stencil);
}

} // namespace glapi


/*
SECTION: Pixel Storage Modes and PBOs [8.4.1, 18.2.2].

Parameter Name                 Type    Initial Value    Valid Range
-------------------------------------------------------------------
UNPACK_SWAP_BYTES              boolean FALSE            TRUE/FALSE
UNPACK_LSB_FIRST               boolean FALSE            TRUE/FALSE
UNPACK_ROW_LENGTH              integer 0                [0, ∞)
UNPACK_SKIP_ROWS               integer 0                [0, ∞)
UNPACK_SKIP_PIXELS             integer 0                [0, ∞)
UNPACK_ALIGNMENT               integer 4                1,2,4,8
UNPACK_IMAGE_HEIGHT            integer 0                [0, ∞)
UNPACK_SKIP_IMAGES             integer 0                [0, ∞)
UNPACK_COMPRESSED_BLOCK_WIDTH  integer 0                [0, ∞)
UNPACK_COMPRESSED_BLOCK_HEIGHT integer 0                [0, ∞)
UNPACK_COMPRESSED_BLOCK_DEPTH  integer 0                [0, ∞)
UNPACK_COMPRESSED_BLOCK_SIZE   integer 0                [0, ∞)

Parameter Name                 Type    Initial Value    Valid Range
-------------------------------------------------------------------
PACK_SWAP_BYTES                boolean FALSE            TRUE/FALSE
PACK_LSB_FIRST                 boolean FALSE            TRUE/FALSE
PACK_ROW_LENGTH                integer 0                [0, ∞)
PACK_SKIP_ROWS                 integer 0                [0, ∞)
PACK_SKIP_PIXELS               integer 0                [0, ∞)
PACK_ALIGNMENT                 integer 4                1,2,4,8
PACK_IMAGE_HEIGHT              integer 0                [0, ∞)
PACK_SKIP_IMAGES               integer 0                [0, ∞)
PACK_COMPRESSED_BLOCK_WIDTH    integer 0                [0, ∞)
PACK_COMPRESSED_BLOCK_HEIGHT   integer 0                [0, ∞)
PACK_COMPRESSED_BLOCK_DEPTH    integer 0                [0, ∞)
PACK_COMPRESSED_BLOCK_SIZE     integer 0                [0, ∞)
*/
namespace glapi {

#define JOSH3D_DEFINE_PIXEL_PACK_BOOL_FUNCS(FName, PName)    \
    inline void set_##FName(bool value)  {           \
        gl::glPixelStorei(gl::GL_##PName, GLboolean(value)); \
    }                                                        \
    inline bool get_##FName()  {                     \
        return detail::get_boolean(gl::GL_##PName);          \
    }

JOSH3D_DEFINE_PIXEL_PACK_BOOL_FUNCS(pixel_unpack_swap_bytes, UNPACK_SWAP_BYTES)
JOSH3D_DEFINE_PIXEL_PACK_BOOL_FUNCS(pixel_unpack_lsb_first,  UNPACK_LSB_FIRST )

JOSH3D_DEFINE_PIXEL_PACK_BOOL_FUNCS(pixel_pack_swap_bytes,   PACK_SWAP_BYTES  )
JOSH3D_DEFINE_PIXEL_PACK_BOOL_FUNCS(pixel_pack_lsb_first,    PACK_LSB_FIRST   )

#define JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(FName, PName) \
    inline void set_##FName(GLint value)  {      \
        gl::glPixelStorei(gl::GL_##PName, value);        \
    }                                                    \
    inline GLint get_##FName()  {                \
        return detail::get_integer(gl::GL_##PName);      \
    }

JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_row_length,              UNPACK_ROW_LENGTH             )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_skip_rows,               UNPACK_SKIP_ROWS              )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_skip_pixels,             UNPACK_SKIP_PIXELS            )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_alignment,               UNPACK_ALIGNMENT              )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_image_height,            UNPACK_IMAGE_HEIGHT           )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_skip_images,             UNPACK_SKIP_IMAGES            )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_compressed_block_width,  UNPACK_COMPRESSED_BLOCK_WIDTH )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_compressed_block_height, UNPACK_COMPRESSED_BLOCK_HEIGHT)
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_compressed_block_depth,  UNPACK_COMPRESSED_BLOCK_DEPTH )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_unpack_compressed_block_size,   UNPACK_COMPRESSED_BLOCK_SIZE  )

JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_row_length,                PACK_ROW_LENGTH             )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_skip_rows,                 PACK_SKIP_ROWS              )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_skip_pixels,               PACK_SKIP_PIXELS            )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_alignment,                 PACK_ALIGNMENT              )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_image_height,              PACK_IMAGE_HEIGHT           )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_skip_images,               PACK_SKIP_IMAGES            )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_compressed_block_width,    PACK_COMPRESSED_BLOCK_WIDTH )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_compressed_block_height,   PACK_COMPRESSED_BLOCK_HEIGHT)
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_compressed_block_depth,    PACK_COMPRESSED_BLOCK_DEPTH )
JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS(pixel_pack_compressed_block_size,     PACK_COMPRESSED_BLOCK_SIZE  )

#undef JOSH3D_DEFINE_PIXEL_PACK_BOOL_FUNCS
#undef JOSH3D_DEFINE_PIXEL_PACK_INT_FUNCS

} // namespace glapi


/*
SECTION: Reading Pixels [18.2].
*/
enum class ReadColorClamping : GLuint
{
    Enabled   = GLuint(gl::GL_TRUE),
    Disabled  = GLuint(gl::GL_FALSE),
    FixedOnly = GLuint(gl::GL_FIXED_ONLY),
};
JOSH3D_DEFINE_ENUM_EXTRAS(ReadColorClamping, Enabled, Disabled, FixedOnly);

namespace glapi {

inline void read_pixels_into(
    BindToken<Binding::ReadFramebuffer> bound_read_framebuffer [[maybe_unused]],
    const Region2I&                     region,
    PixelDataFormat                     format,
    PixelDataType                       type,
    std::span<GLubyte>                  dst_buf)
{
    assert(get_bound_id(Binding::ReadFramebuffer) == bound_read_framebuffer.id());
    assert(get_bound_id(Binding::PixelPackBuffer) == 0);
    gl::glReadnPixels(
        region.offset.x,     region.offset.y,
        region.extent.width, region.extent.height,
        enum_cast<GLenum>(format), enum_cast<GLenum>(type),
        GLsizei(dst_buf.size()),
        dst_buf.data());
}

template<specifies_pixel_pack_traits PixelT>
inline void read_pixels_into(
    BindToken<Binding::ReadFramebuffer> bound_read_framebuffer [[maybe_unused]],
    const Region2I&                     region,
    std::span<PixelT>                   dst_buf)
{
    using pptr = pixel_pack_traits<PixelT>;
    read_pixels_into(bound_read_framebuffer, region, pptr::format, pptr::type, dst_buf);
}

inline void read_pixels_to_pixel_pack_buffer(
    BindToken<Binding::ReadFramebuffer> bound_read_framebuffer [[maybe_unused]],
    const Region2I&                     region,
    PixelDataFormat                     format,
    PixelDataType                       type,
    BindToken<Binding::PixelPackBuffer> bound_pack_buffer [[maybe_unused]],
    OffsetBytes                         offset_bytes)
{
    assert(get_bound_id(Binding::ReadFramebuffer) == bound_read_framebuffer.id());
    assert(get_bound_id(Binding::PixelPackBuffer) == bound_pack_buffer.id());
    gl::glReadPixels(
        region.offset.x,     region.offset.y,
        region.extent.width, region.extent.height,
        enum_cast<GLenum>(format), enum_cast<GLenum>(type),
        reinterpret_cast<void*>(offset_bytes.value)); // NOLINT
}

inline void set_read_color_clamping(ReadColorClamping clamping)
{
    // FIXME: glbinding takes GLboolean and GL_FIXED_ONLY is impossible to pass.
    // This is fixed in later versions so we should probably upgrade.
    gl::glClampColor(gl::GL_CLAMP_READ_COLOR, clamping == ReadColorClamping::Enabled);
}

inline auto get_read_color_clamping()
    -> ReadColorClamping
{
    // We return full set: TRUE, FALSE and FIXED_ONLY as that's safer.
    // Keep in mind that the default value for CLAMP_READ_COLOR is exactly FIXED_ONLY.
    return detail::get_enum<ReadColorClamping>(gl::GL_CLAMP_READ_COLOR);
}

} // namespace glapi


/*
SECTION: Compute Shaders [19].

TODO: What is here other than limits?
*/


/*
SECTION: Primitive Restart.
*/
namespace glapi {

constexpr GLubyte  primitive_restart_fixed_ubyte_index  = -1;
constexpr GLushort primitive_restart_fixed_ushort_index = -1;
constexpr GLuint   primitive_restart_fixed_uint_index   = -1;

inline void set_primitive_restart_index(GLuint restart_index)
{
    gl::glPrimitiveRestartIndex(restart_index);
}

inline auto get_primitive_restart_index()
    -> GLuint
{
    return detail::get_integer(gl::GL_PRIMITIVE_RESTART_INDEX);
}

} // namespace glapi


/*
SECTION: Generic Vertex Attributes.
*/
namespace glapi {

/*
NOTE: This function declaration is provided for exposition only.

Setting generic vertex attributes is not implemented.
If this functionality is desired, `glVertexAttrib*` functions should be
called directly.

If you are still not sure if you need this, here is a wiki quote to discourage you more:
"Note: It is not recommended that you use these. The performance characteristics of using
fixed attribute data are unknown, and it is not a high-priority case that OpenGL driver developers
optimize for. They might be faster than uniforms, or they might not."
*/
void set_generic_vertex_attribute() = delete;

/*
NOTE: This function declaration is provided for exposition only.

Querying generic vertex attributes is not implemented.
If this functionality is desired, `glGetVertexAttrib*` functions should be
called directly.

If you are still not sure if you need this, here is a wiki quote to discourage you more:
"Note: It is not recommended that you use these. The performance characteristics of using
fixed attribute data are unknown, and it is not a high-priority case that OpenGL driver developers
optimize for. They might be faster than uniforms, or they might not."
*/
auto get_generic_vertex_attribute() = delete;

} // namespace glapi


/*
SECTION: Conditional Rendering.

"[4.6, 10.9] If the result (SAMPLES_PASSED) of the query is zero, or if the result
(ANY_SAMPLES_PASSED, ANY_SAMPLES_PASSED_CONSERVATIVE, TRANSFORM_FEEDBACK_OVERFLOW,
or TRANSFORM_FEEDBACK_STREAM_OVERFLOW) is FALSE, all rendering commands
described in section 2.4 are discarded and have no effect when issued between
BeginConditionalRender and the corresponding EndConditionalRender."
*/
enum class ConditionalRenderQueryResult
{
    Wait      = (0u << 0),
    DoNotWait = (1u << 0),
};
JOSH3D_DEFINE_ENUM_EXTRAS(ConditionalRenderQueryResult, Wait, DoNotWait);

enum class ConditionalRenderOccludedRegion
{
    CannotDiscard = (0u << 1),
    CanDiscard    = (1u << 1),
};
JOSH3D_DEFINE_ENUM_EXTRAS(ConditionalRenderOccludedRegion, CannotDiscard, CanDiscard);

enum class ConditionalRenderCondition
{
    Normal   = (0u << 2),
    Inverted = (1u << 2),
};
JOSH3D_DEFINE_ENUM_EXTRAS(ConditionalRenderCondition, Normal, Inverted);

struct ConditionalRenderParams
{
    ConditionalRenderQueryResult    result_mode    = ConditionalRenderQueryResult::DoNotWait;
    ConditionalRenderOccludedRegion region_mode    = ConditionalRenderOccludedRegion::CannotDiscard;
    ConditionalRenderCondition      condition_mode = ConditionalRenderCondition::Normal;
};

namespace glapi {
namespace detail {
/*
This is only for the implementation to quickly identify a combination of flags.
*/
enum class ConditionalRenderMode
{   //                                                                NOWAIT     DISCARD    INVERTED
    WaitForQueryResult                                              = (0 << 0) | (0 << 1) | (0 << 2),
    NoWaitForQueryResult                                            = (1 << 0) | (0 << 1) | (0 << 2),
    WaitForQueryResult_CanDiscardOccludedRegion                     = (0 << 0) | (1 << 1) | (0 << 2),
    NoWaitForQueryResult_CanDiscardOccludedRegion                   = (1 << 0) | (1 << 1) | (0 << 2),
    WaitForQueryResult_InvertedCondition                            = (0 << 0) | (0 << 1) | (1 << 2),
    NoWaitForQueryResult_InvertedCondition                          = (1 << 0) | (0 << 1) | (1 << 2),
    WaitForQueryResult_CanDiscardOccludedRegion_InvertedCondition   = (0 << 0) | (1 << 1) | (1 << 2),
    NoWaitForQueryResult_CanDiscardOccludedRegion_InvertedCondition = (1 << 0) | (1 << 1) | (1 << 2),
};
} // namespace detail

template<of_kind<GLKind::Query> QueryT>
inline void begin_conditional_render(
    const QueryT&                  query,
    const ConditionalRenderParams& params = {})
        requires
            // A little awkward...
            (QueryT::target_type == QueryTarget::SamplesPassed)                   ||
            (QueryT::target_type == QueryTarget::AnySamplesPassed)                ||
            (QueryT::target_type == QueryTarget::AnySamplesPassedConservative)    ||
            (QueryT::target_type == QueryTarget::TransformFeedbackOverflow)       ||
            (QueryT::target_type == QueryTarget::TransformFeedbackStreamOverflow)
{
    const auto [result_mode, region_mode, condition_mode] = params;
    const auto mode = detail::ConditionalRenderMode{
        to_underlying(result_mode) |
        to_underlying(region_mode) |
        to_underlying(condition_mode)
    };
    // NOTE: The API apparently did not use a bitset here.
    const GLenum real_mode = [&]() -> GLenum {
        switch (mode)
        {
            using enum detail::ConditionalRenderMode;
            case WaitForQueryResult:
                return gl::GL_QUERY_WAIT;
            case NoWaitForQueryResult:
                return gl::GL_QUERY_NO_WAIT;
            case WaitForQueryResult_CanDiscardOccludedRegion:
                return gl::GL_QUERY_BY_REGION_WAIT;
            case NoWaitForQueryResult_CanDiscardOccludedRegion:
                return gl::GL_QUERY_BY_REGION_NO_WAIT;
            case WaitForQueryResult_InvertedCondition:
                return gl::GL_QUERY_WAIT_INVERTED;
            case NoWaitForQueryResult_InvertedCondition:
                return gl::GL_QUERY_NO_WAIT_INVERTED;
            case WaitForQueryResult_CanDiscardOccludedRegion_InvertedCondition:
                return gl::GL_QUERY_BY_REGION_WAIT_INVERTED;
            case NoWaitForQueryResult_CanDiscardOccludedRegion_InvertedCondition:
                return gl::GL_QUERY_BY_REGION_NO_WAIT_INVERTED;
            default:
                assert(false); return {};
        };
    }();
    gl::glBeginConditionalRender(decay_to_raw(query).id(), real_mode);
}

inline void end_conditional_render()
{
    gl::glEndConditionalRender();
}

} // namespace glapi


} // namespace josh
