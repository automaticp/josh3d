#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPIQueries.hpp"
#include "GLDSAQueries.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "Index.hpp"
#include "Region.hpp"
#include "Size.hpp"
#include "detail/GLAPIGet.hpp"
#include "detail/StrongScalar.hpp"
#include <cassert>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <span>




namespace josh {


struct RangeF {
    float min, max;
};







// Section: Draw and Dispatch.


enum class Primitive : GLuint {
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


enum class ElementType : GLuint {
    UByte  = GLuint(gl::GL_UNSIGNED_BYTE),
    UShort = GLuint(gl::GL_UNSIGNED_SHORT),
    UInt   = GLuint(gl::GL_UNSIGNED_INT),
};


namespace detail {
template<ElementType ElemT> constexpr auto element_size() noexcept;
template<> constexpr auto element_size<ElementType::UByte>() noexcept  { return sizeof(GLubyte);  }
template<> constexpr auto element_size<ElementType::UShort>() noexcept { return sizeof(GLushort); }
template<> constexpr auto element_size<ElementType::UInt>() noexcept   { return sizeof(GLuint);   }
} // namespace detail {


namespace glapi {


inline void draw_arrays(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    GLint                               vertex_offset,
    GLsizei                             vertex_count) noexcept
{
    assert(bound_program.id()          == queries::bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == queries::bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == queries::bound_id(Binding::VertexArray));
    gl::glDrawArrays(enum_cast<GLenum>(primitive), vertex_offset, vertex_count);
}


inline void draw_elements(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    ElementType                         type,
    GLsizeiptr                          element_offset_bytes,
    GLsizei                             element_count) noexcept
{
    assert(bound_program.id()          == queries::bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == queries::bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == queries::bound_id(Binding::VertexArray));
    gl::glDrawElements(
        enum_cast<GLenum>(primitive),
        element_count, enum_cast<GLenum>(type),
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        reinterpret_cast<const void*>(element_offset_bytes)
    );
}


inline void multidraw_arrays(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    // TODO: Arguments are easily confused.
    std::span<const GLint>              vertex_offsets,
    std::span<const GLsizei>            vertex_counts) noexcept
{
    assert(bound_program.id()          == queries::bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == queries::bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == queries::bound_id(Binding::VertexArray));
    assert(vertex_offsets.size() == vertex_counts.size());
    gl::glMultiDrawArrays(
        enum_cast<GLenum>(primitive),
        vertex_offsets.data(),
        vertex_counts.data(),
        static_cast<GLsizei>(vertex_counts.size())
    );
}


inline void multidraw_elements(
    BindToken<Binding::VertexArray>     bound_vertex_array     [[maybe_unused]],
    BindToken<Binding::Program>         bound_program          [[maybe_unused]],
    BindToken<Binding::DrawFramebuffer> bound_draw_framebuffer [[maybe_unused]],
    Primitive                           primitive,
    ElementType                         type,
    // TODO: Arguments are easily confused.
    std::span<const GLsizeiptr>         element_offsets_bytes,
    std::span<const GLsizei>            element_counts) noexcept
{
    assert(bound_program.id()          == queries::bound_id(Binding::Program));
    assert(bound_draw_framebuffer.id() == queries::bound_id(Binding::DrawFramebuffer));
    assert(bound_vertex_array.id()     == queries::bound_id(Binding::VertexArray));
    assert(element_offsets_bytes.size() == element_counts.size());
    using offset_t = const void*;
    gl::glMultiDrawElements(
        enum_cast<GLenum>(primitive),
        element_counts.data(),
        enum_cast<GLenum>(type),
        reinterpret_cast<const offset_t*>(element_offsets_bytes.data()),
        static_cast<GLsizei>(element_counts.size())
    );
}


inline void _draw_elements_basevertex() noexcept {
    // gl::glDrawElementsBaseVertex()
}

inline void _multidraw_elements_basevertex() noexcept {
    // gl::glMultiDrawElementsBaseVertex()
}


inline void _draw_arrays_instanced() noexcept {
    // gl::glDrawArraysInstanced()
}

inline void _draw_arrays_instanced_baseinstance() noexcept {
    // gl::glDrawArraysInstancedBaseInstance()
}


inline void _draw_elements_instanced() noexcept {
    // gl::glDrawElementsInstanced()
}

inline void _draw_elements_instanced_baseinstance() noexcept {
    // gl::glDrawElementsInstancedBaseInstance()
}

inline void _draw_elements_instanced_basevertex_baseinstance() noexcept {
    // gl::glDrawElementsInstancedBaseVertexBaseInstance()
}


inline void _draw_elements_range() noexcept {
    // gl::glDrawRangeElements()
}

inline void _draw_elements_range_basevertex() noexcept {
    // gl::glDrawRangeElementsBaseVertex()
}



namespace limits {
// TODO: What's a more correct name?
inline auto _recommended_max_num_vertices_per_draw() noexcept {
    return detail::get_integer(gl::GL_MAX_ELEMENTS_VERTICES);
}
inline auto _recommended_max_num_indices_per_draw() noexcept {
    return detail::get_integer(gl::GL_MAX_ELEMENTS_VERTICES);
}
} // namespace limits





inline void _dispatch_compute() noexcept {
    // gl::glDispatchCompute()
}


// TODO: Indirect
// TODO: TransformFeedbacks


} // namespace glapi


// For exposition only.
struct DrawArraysIndirectCommand {
    GLuint vertex_count;
    GLuint instance_count;
    GLuint vertex_offset;
    GLuint base_instance;
};


// For exposition only.
struct DrawElementsIndirectCommand {
    GLuint element_count;
    GLuint instance_count;
    GLuint element_offset;
    GLuint base_vertex;
    GLuint base_instance;
};


// For exposition only.
struct DispatchIndirectCommand {
    GLuint num_groups_x;
    GLuint num_groups_y;
    GLuint num_groups_z;
};


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


namespace glapi {


/*
An INVALID_VALUE error is generated if indirect is not a multiple of the
size, in basic machine units, of uint.
*/
inline void _draw_elements_indirect() noexcept {
    // gl::glDrawElementsIndirect()
}

inline void _multidraw_elements_indirect() noexcept {
    // gl::glMultiDrawElementsIndirect()
}

inline void _multidraw_elements_indirect_count() noexcept {
    // gl::glMultiDrawElementsIndirectCount()
}


} // namespace glapi














// Section: Capabilities.


enum class Capability : GLuint {

    SeamlessCubemaps           = GLuint(gl::GL_TEXTURE_CUBE_MAP_SEAMLESS),

    PrimitiveRestart           = GLuint(gl::GL_PRIMITIVE_RESTART),
    PrimitiveRestartFixedIndex = GLuint(gl::GL_PRIMITIVE_RESTART_FIXED_INDEX),

    DiscardRasterizer          = GLuint(gl::GL_RASTERIZER_DISCARD),

    ScissorTesting             = GLuint(gl::GL_SCISSOR_TEST),

    StencilTesting             = GLuint(gl::GL_STENCIL_TEST),

    DepthTesting               = GLuint(gl::GL_DEPTH_TEST),

    Blending                   = GLuint(gl::GL_BLEND),

    Multisampling              = GLuint(gl::GL_MULTISAMPLE),
    SampleShading              = GLuint(gl::GL_SAMPLE_SHADING),

    ProgramSpecifiedPointSize  = GLuint(gl::GL_PROGRAM_POINT_SIZE),

    AntialiasedLines           = GLuint(gl::GL_LINE_SMOOTH),
    AntialiasedPolygons        = GLuint(gl::GL_POLYGON_SMOOTH),

    FaceCulling                = GLuint(gl::GL_CULL_FACE),

    PolygonOffsetPoint         = GLuint(gl::GL_POLYGON_OFFSET_POINT),
    PolygonOffsetLine          = GLuint(gl::GL_POLYGON_OFFSET_LINE),
    PolygonOffsetFill          = GLuint(gl::GL_POLYGON_OFFSET_FILL),
};


enum class CapabilityIndexed : GLuint {

    ScissorTest                = GLuint(gl::GL_SCISSOR_TEST),
    Blending                   = GLuint(gl::GL_BLEND),
};


namespace glapi {


inline void enable(Capability cap) noexcept {
    gl::glEnable(enum_cast<GLenum>(cap));
}

inline void disable(Capability cap) noexcept {
    gl::glDisable(enum_cast<GLenum>(cap));
}

inline bool is_enabled(Capability cap) noexcept {
    return bool(gl::glIsEnabled(enum_cast<GLenum>(cap)));
}


inline void enable_indexed(CapabilityIndexed cap, GLuint index) noexcept {
    gl::glEnablei(enum_cast<GLenum>(cap), index);
}

inline void disable_indexed(CapabilityIndexed cap, GLuint index) noexcept {
    gl::glDisablei(enum_cast<GLenum>(cap), index);
}

inline bool is_enabled_indexed(CapabilityIndexed cap, GLuint index) noexcept {
    return bool(gl::glIsEnabledi(enum_cast<GLenum>(cap), index));
}





} // namespace glapi






// Section: Multisampling.

namespace glapi {

// The location in pixel space at which shading is performed for a given sample.
// Pair of values in range [0, 1]. Pixel center is { 0.5, 0.5 }.
inline auto get_sample_shading_location(GLuint sample_index) noexcept
    -> Offset2F
{
    float offsets[2];
    gl::glGetMultisamplefv(gl::GL_SAMPLE_POSITION, sample_index, offsets);
    return { offsets[0], offsets[1] };
}

// When both `Multisampling` and `SampleShading` are enabled, then
// each fragment shader invocation recieves, at minimum, a number of samples equal to:
// `max(ssr * samples, 1)`, where `ssr` is sample shading rate.
//
// The value of `rate` is clamped to the range of [0, 1].
inline void set_sample_shading_rate(GLfloat rate) noexcept {
    gl::glMinSampleShading(rate);
}

inline GLfloat get_sample_shading_rate() noexcept {
    return detail::get_float(gl::GL_MIN_SAMPLE_SHADING_VALUE);
}


} // namespace glapi








// Section: Point Rasterization Parameters.



enum class PointSpriteCoordOrigin : GLuint {
    LowerLeft = GLuint(gl::GL_LOWER_LEFT),
    UpperLeft = GLuint(gl::GL_UPPER_LEFT),
};


namespace glapi {

inline void set_point_size(GLfloat size) noexcept {
    gl::glPointSize(size);
}

inline GLfloat get_point_size() noexcept {
    return detail::get_float(gl::GL_POINT_SIZE);
}

inline void set_point_fade_threshold(GLfloat threshold) noexcept {
    gl::glPointParameterf(gl::GL_POINT_FADE_THRESHOLD_SIZE, threshold);
}

inline GLfloat get_point_fade_threshold() noexcept {
    return detail::get_float(gl::GL_POINT_FADE_THRESHOLD_SIZE);
}

inline void set_point_sprite_coordinate_origin(PointSpriteCoordOrigin origin) noexcept {
    gl::glPointParameteri(gl::GL_POINT_SPRITE_COORD_ORIGIN, enum_cast<GLenum>(origin));
}

inline auto get_point_sprite_coordinate_origin() noexcept
    -> PointSpriteCoordOrigin
{
    return detail::get_enum<PointSpriteCoordOrigin>(gl::GL_POINT_SPRITE_COORD_ORIGIN);
}

namespace limits {
// TODO: There are separate verisions for smooth points it seems

inline RangeF point_size_range() noexcept {
    auto [min, max] = detail::get_floatv<2>(gl::GL_POINT_SIZE_RANGE);
    return { min, max };
}

inline GLfloat point_size_granularity() noexcept {
    return detail::get_float(gl::GL_POINT_SIZE_GRANULARITY);
}

} // namespace limits
} // namespace glapi







// Section: Line Rasterization Parameters.


namespace glapi {

inline void set_line_width(GLfloat width) noexcept {
    gl::glLineWidth(width);
}

inline GLfloat get_line_width() noexcept {
    return detail::get_float(gl::GL_LINE_WIDTH);
}

namespace limits {
inline RangeF line_width_range() noexcept {
    auto [min, max] = detail::get_floatv<2>(gl::GL_LINE_WIDTH_RANGE);
    return { min, max };
}

inline GLfloat line_width_granularity() noexcept {
    return detail::get_float(gl::GL_LINE_WIDTH_GRANULARITY);
}

} // namespace limits
} // namespace glapi








// Section: Polygon Rasterization Parameters.

// TODO: Clip Control


enum class WindingOrder : GLuint {
    CounterClockwise = GLuint(gl::GL_CCW),
    Clockwise        = GLuint(gl::GL_CW),
};


enum class Faces : GLuint {
    Front        = GLuint(gl::GL_FRONT),
    Back         = GLuint(gl::GL_BACK),
    FrontAndBack = GLuint(gl::GL_FRONT_AND_BACK),
};


/*
"[14.6.4] Polygon antialiasing applies only to the FILL state of PolygonMode. For
POINT or LINE, point antialiasing or line segment antialiasing, respectively, apply."
*/
enum class PolygonRaserization : GLuint {
    Point = GLuint(gl::GL_POINT),
    Line  = GLuint(gl::GL_LINE),
    Fill  = GLuint(gl::GL_FILL),
};


namespace glapi {

inline void set_front_face_winding_order(WindingOrder order) noexcept {
    gl::glFrontFace(enum_cast<GLenum>(order));
}

inline auto get_front_face_winding_order() noexcept
    -> WindingOrder
{
    return detail::get_enum<WindingOrder>(gl::GL_FRONT_FACE);
}

inline void set_face_culling_target(Faces culled_faces) noexcept {
    gl::glCullFace(enum_cast<GLenum>(culled_faces));
}

inline auto get_face_culling_target() noexcept
    -> Faces
{
    return detail::get_enum<Faces>(gl::GL_CULL_FACE_MODE);
}

inline void set_polygon_rasterization_mode(PolygonRaserization mode) noexcept {
    gl::glPolygonMode(gl::GL_FRONT_AND_BACK, enum_cast<GLenum>(mode));
}

inline auto get_polygon_rasterization_mode() noexcept
    -> PolygonRaserization
{
    return detail::get_enum<PolygonRaserization>(gl::GL_POLYGON_MODE);
}

inline auto set_polygon_offset_clamped(
    GLfloat slope_factor, GLfloat bias_scale, GLfloat bias_clamp) noexcept
{
    gl::glPolygonOffsetClamp(slope_factor, bias_scale, bias_clamp);
}

inline auto set_polygon_offset(
    GLfloat slope_factor, GLfloat bias_scale) noexcept
{
    gl::glPolygonOffset(slope_factor, bias_scale);
}

inline GLfloat get_polygon_offset_slope_factor() noexcept {
    return detail::get_float(gl::GL_POLYGON_OFFSET_FACTOR);
}

inline GLfloat get_polygon_offset_bias_scale() noexcept {
    return detail::get_float(gl::GL_POLYGON_OFFSET_UNITS);
}

inline GLfloat get_polygon_offset_bias_clamp() noexcept {
    return detail::get_float(gl::GL_POLYGON_OFFSET_CLAMP);
}

} // namespace glapi









// Section: Scissor Test [???].


namespace glapi {


inline void set_scissor_region(const Region2I& region) noexcept {
    gl::glScissor(
        region.offset.x, region.offset.y, region.extent.width, region.extent.height
    );
}

inline void set_scissor_region_indexed(
    GLuint viewport_index, const Region2I& region) noexcept
{
    gl::glScissorIndexed(
        viewport_index,
        region.offset.x, region.offset.y, region.extent.width, region.extent.height
    );
}

inline void set_scissor_regions(
    GLuint first_viewport_index, std::span<const Region2I> regions) noexcept
{
    gl::glScissorArrayv(
        first_viewport_index,
        GLsizei(regions.size()), // This isn't `size_bytes() / sizeof(int)`.
        // Shooo! UB go away.
        std::launder(reinterpret_cast<const GLint*>(regions.data()))
    );
}

// TODO: Does this work?
inline auto get_scissor_region() noexcept
    -> Region2I
{
    auto [x, y, w, h] = detail::get_integerv<4>(gl::GL_SCISSOR_BOX);
    return { { x, y }, { w, h } };
}

inline auto get_scissor_region_indexed(GLuint viewport_index) noexcept
    -> Region2I
{
    auto [x, y, w, h] = detail::get_integerv_indexed<4>(gl::GL_SCISSOR_BOX, viewport_index);
    return { { x, y }, { w, h } };
}


} // namespace glapi








// Section: Multisample Fragment Operations [14.9.3].

// TODO





// Section: Alpha To Coverage [17.3.1].

// TODO






// Section: Stencil Test [17.3.3].


// TODO: The same enum is used in Textures. Keep one.
enum class CompareOp : GLuint {
    LEqual   = GLuint(gl::GL_LEQUAL),
    GEqual   = GLuint(gl::GL_GEQUAL),
    Less     = GLuint(gl::GL_LESS),
    Greater  = GLuint(gl::GL_GREATER),
    Equal    = GLuint(gl::GL_EQUAL),
    NotEqual = GLuint(gl::GL_NOTEQUAL),
    Always   = GLuint(gl::GL_ALWAYS),
    Never    = GLuint(gl::GL_NEVER),
};


enum class StencilOp : GLuint {
    Keep              = GLuint(gl::GL_KEEP),
    SetZero           = GLuint(gl::GL_ZERO),
    ReplaceWithRef    = GLuint(gl::GL_REPLACE),
    IncrementSaturate = GLuint(gl::GL_INCR),
    DecrementSaturate = GLuint(gl::GL_DECR),
    BitwiseInvert     = GLuint(gl::GL_INVERT),
    IncrementWrap     = GLuint(gl::GL_INCR_WRAP),
    DecrementWrap     = GLuint(gl::GL_DECR_WRAP),
};


enum class Face : GLuint {
    Front = GLuint(gl::GL_FRONT),
    Back  = GLuint(gl::GL_BACK),
};


JOSH3D_DEFINE_STRONG_SCALAR(Mask, GLuint)


namespace glapi {

// Stencil Test Pass = Ref & RefMask [op] Stored Stencil Value
inline void set_stencil_test_condition(
    Mask      ref_mask,
    GLint     ref,
    CompareOp op) noexcept
{
    gl::glStencilFunc(
        enum_cast<GLenum>(op),
        ref, ref_mask
    );
}

inline void set_stencil_test_condition_per_face(
    Face      face,
    Mask      ref_mask,
    GLint     ref,
    CompareOp op) noexcept
{
    gl::glStencilFuncSeparate(
        enum_cast<GLenum>(face),
        enum_cast<GLenum>(op),
        ref, ref_mask
    );
}

inline void set_stencil_test_operations(
    StencilOp on_stencil_fail,
    StencilOp on_stencil_pass_depth_fail,
    StencilOp on_stencil_pass_depth_pass) noexcept
{
    gl::glStencilOp(
        enum_cast<GLenum>(on_stencil_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_pass)
    );
}

inline void set_stencil_test_operations_per_face(
    Face      face,
    StencilOp on_stencil_fail,
    StencilOp on_stencil_pass_depth_fail,
    StencilOp on_stencil_pass_depth_pass) noexcept
{
    gl::glStencilOpSeparate(
        enum_cast<GLenum>(face),
        enum_cast<GLenum>(on_stencil_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_fail),
        enum_cast<GLenum>(on_stencil_pass_depth_pass)
    );
}

inline auto get_stencil_test_condition_comapre_op(Face face) noexcept
    -> CompareOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_FUNC : gl::GL_STENCIL_BACK_FUNC };
    return detail::get_enum<CompareOp>(pname);
}

inline GLint get_stencil_test_condition_ref(Face face) noexcept {
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_REF : gl::GL_STENCIL_BACK_REF };
    return detail::get_integer(pname);
}

inline GLuint get_stencil_test_condition_ref_mask(Face face) noexcept {
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_VALUE_MASK : gl::GL_STENCIL_BACK_VALUE_MASK };
    return detail::get_integer(pname);
}

inline auto get_stencil_test_operation_on_stencil_fail(Face face) noexcept
    -> StencilOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_FAIL : gl::GL_STENCIL_BACK_FAIL };
    return detail::get_enum<StencilOp>(pname);
}

inline auto get_stencil_test_operation_on_stencil_pass_depth_fail(Face face) noexcept
    -> StencilOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_PASS_DEPTH_FAIL : gl::GL_STENCIL_BACK_PASS_DEPTH_FAIL };
    return detail::get_enum<StencilOp>(pname);
}

inline auto get_stencil_test_operation_on_stencil_pass_depth_pass(Face face) noexcept
    -> StencilOp
{
    GLenum pname{ face == Face::Front ? gl::GL_STENCIL_PASS_DEPTH_PASS : gl::GL_STENCIL_BACK_PASS_DEPTH_PASS };
    return detail::get_enum<StencilOp>(pname);
}


} // namespace glapi











// Section: Depth Buffer Test [17.3.4].


namespace glapi {

// Depth Test Pass = Incoming Depth [op] Stored Depth.
inline void set_depth_test_condition(CompareOp op) noexcept {
    gl::glDepthFunc(enum_cast<GLenum>(op));
}

inline auto get_depth_test_condition_compare_op() noexcept
    -> CompareOp
{
    return detail::get_enum<CompareOp>(gl::GL_DEPTH_FUNC);
}

// TODO: Depth Clamping [13.7], Depth Range [13.8]...


} // namespace glapi









// Section: Blending [17.3.6].


// TODO:

enum class BlendEquation : GLuint {
    FuncAdd             = GLuint(gl::GL_FUNC_ADD),
    FuncSubtract        = GLuint(gl::GL_FUNC_SUBTRACT),
    FuncReverseSubtract = GLuint(gl::GL_FUNC_REVERSE_SUBTRACT),
    Min                 = GLuint(gl::GL_MIN),
    Max                 = GLuint(gl::GL_MAX),
};


enum class BlendFactor : GLuint {
};


namespace glapi {

inline void _set_blend_equation() noexcept;
inline void _set_blend_factors() noexcept;
inline void _set_blend_constant_color() noexcept;


} // namespace glapi







// Section: Primitive Restart.


namespace glapi {


static constexpr GLubyte  primitive_restart_fixed_ubyte_index  = -1;
static constexpr GLushort primitive_restart_fixed_ushort_index = -1;
static constexpr GLuint   primitive_restart_fixed_uint_index   = -1;


inline void set_primitive_restart_index(GLuint restart_index) noexcept {
    gl::glPrimitiveRestartIndex(restart_index);
}

inline GLuint get_primitive_restart_index() noexcept {
    return detail::get_integer(gl::GL_PRIMITIVE_RESTART_INDEX);
}


namespace limits {
inline bool is_primitive_restart_supported_for_patches() noexcept {
    return detail::get_boolean(gl::GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED);
}
} // namespace limits


} // namespace glapi








// Section: Generic Vertex Attributes.


namespace glapi {


// This function declaration is provided for exposition only.
//
// Setting generic vertex attributes is not implemented.
// If this functionality is desired, `glVertexAttrib*` functions should be
// called directly.
//
// If you are still not sure if you need this, here is a wiki quote to discourage you more:
// "Note: It is not recommended that you use these. The performance characteristics of using
// fixed attribute data are unknown, and it is not a high-priority case that OpenGL driver developers
// optimize for. They might be faster than uniforms, or they might not."
inline void set_generic_vertex_attribute() noexcept;

// This function declaration is provided for exposition only.
//
// Querying generic vertex attributes is not implemented.
// If this functionality is desired, `glGetVertexAttrib*` functions should be
// called directly.
//
// If you are still not sure if you need this, here is a wiki quote to discourage you more:
// "Note: It is not recommended that you use these. The performance characteristics of using
// fixed attribute data are unknown, and it is not a high-priority case that OpenGL driver developers
// optimize for. They might be faster than uniforms, or they might not."
inline auto get_generic_vertex_attribute() noexcept;


namespace limits {
inline auto max_vertex_attributes() noexcept {
    return detail::get_integer(gl::GL_MAX_VERTEX_ATTRIBS);
}
inline auto max_vertex_buffer_attachment_slots() noexcept {
    return detail::get_integer(gl::GL_MAX_VERTEX_ATTRIB_BINDINGS);
}
} // namespace limits




} // namespace glapi








// Section: Conditional Rendering.

/*
"[4.6, 10.9] If the result (SAMPLES_PASSED) of the query is zero, or if the result
(ANY_SAMPLES_PASSED, ANY_SAMPLES_PASSED_CONSERVATIVE, TRANSFORM_FEEDBACK_OVERFLOW,
or TRANSFORM_FEEDBACK_STREAM_OVERFLOW) is FALSE, all rendering commands
described in section 2.4 are discarded and have no effect when issued between
BeginConditionalRender and the corresponding EndConditionalRender."
*/
enum class ConditionalRenderMode : int {                           // NOWAIT     DISCARD    INVERTED
    WaitForQueryResult                                              = (0 << 0) | (0 << 1) | (0 << 2),
    NoWaitForQueryResult                                            = (1 << 0) | (0 << 1) | (0 << 2),
    WaitForQueryResult_CanDiscardOccludedRegion                     = (0 << 0) | (1 << 1) | (0 << 2),
    NoWaitForQueryResult_CanDiscardOccludedRegion                   = (1 << 0) | (1 << 1) | (0 << 2),
    WaitForQueryResult_InvertedCondition                            = (0 << 0) | (0 << 1) | (1 << 2),
    NoWaitForQueryResult_InvertedCondition                          = (1 << 0) | (0 << 1) | (1 << 2),
    WaitForQueryResult_CanDiscardOccludedRegion_InvertedCondition   = (0 << 0) | (1 << 1) | (1 << 2),
    NoWaitForQueryResult_CanDiscardOccludedRegion_InvertedCondition = (1 << 0) | (1 << 1) | (1 << 2),
};


enum class ConditionalRenderQueryResult : int {
    Wait      = (0u << 0),
    DoNotWait = (1u << 0),
};

enum class ConditionalRenderOccludedRegion : int {
    CannotDiscard = (0u << 1),
    CanDiscard    = (1u << 1),
};

enum class ConditionalRenderCondition : int {
    Normal   = (0u << 2),
    Inverted = (1u << 2),
};


namespace glapi {


template<of_kind<GLKind::Query> QueryT>
inline void begin_conditional_render(
    const QueryT&                   query,
    ConditionalRenderQueryResult    result_mode    = ConditionalRenderQueryResult::DoNotWait,
    ConditionalRenderOccludedRegion region_mode    = ConditionalRenderOccludedRegion::CannotDiscard,
    ConditionalRenderCondition      condition_mode = ConditionalRenderCondition::Normal) noexcept
        requires
            // A little awkward...
            (QueryT::target_type == dsa::QueryTarget::SamplesPassed)                   ||
            (QueryT::target_type == dsa::QueryTarget::AnySamplesPassed)                ||
            (QueryT::target_type == dsa::QueryTarget::AnySamplesPassedConservative)    ||
            (QueryT::target_type == dsa::QueryTarget::TransformFeedbackOverflow)       ||
            (QueryT::target_type == dsa::QueryTarget::TransformFeedbackStreamOverflow)
{
    // I just hope the compiler understands "my intent".
    const auto mode = ConditionalRenderMode{
        enum_cast<int>(result_mode) |
        enum_cast<int>(region_mode) |
        enum_cast<int>(condition_mode)
    };
    const GLenum real_mode = [&]() -> GLenum {
        switch (mode) {
            using enum ConditionalRenderMode;
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
                assert(false);
        };
    }();
    gl::glBeginConditionalRender(query.id(), real_mode);
}


inline void end_conditional_render() noexcept {
    gl::glEndConditionalRender();
}

} // namespace glapi









} // namespace josh
