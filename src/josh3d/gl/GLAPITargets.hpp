#pragma once
#include "GLAPI.hpp"
#include "GLScalars.hpp"
#include "EnumUtils.hpp"


namespace josh {

/*
NOTE: Targets obsoleted by DSA are commented out.
*/
enum class BufferTarget : GLuint
{
    // VertexArray      = GLuint(gl::GL_ARRAY_BUFFER),
    // ElementArray     = GLuint(gl::GL_ELEMENT_ARRAY_BUFFER),
    DrawIndirect     = GLuint(gl::GL_DRAW_INDIRECT_BUFFER),
    DispatchIndirect = GLuint(gl::GL_DISPATCH_INDIRECT_BUFFER),
    Parameter        = GLuint(gl::GL_PARAMETER_BUFFER),
    PixelPack        = GLuint(gl::GL_PIXEL_PACK_BUFFER),
    PixelUnpack      = GLuint(gl::GL_PIXEL_UNPACK_BUFFER),
    // Texture          = GLuint(gl::GL_TEXTURE_BUFFER),
    // QUERY target is redundant in presence of `glGetQueryBufferObjectui64v`.
    // Query            = GLuint(gl::GL_QUERY_BUFFER),
    // COPY_READ/WRITE targets are redundant in presence of DSA copy commands.
    // CopyRead         = GLuint(gl::GL_COPY_READ_BUFFER),
    // CopyWrite        = GLuint(gl::GL_COPY_WRITE_BUFFER),
};
JOSH3D_DEFINE_ENUM_EXTRAS(BufferTarget,
    DispatchIndirect,
    DrawIndirect,
    Parameter,
    PixelPack,
    PixelUnpack);

enum class BufferTargetI : GLuint
{
    ShaderStorage     = GLuint(gl::GL_SHADER_STORAGE_BUFFER),
    Uniform           = GLuint(gl::GL_UNIFORM_BUFFER),
    TransformFeedback = GLuint(gl::GL_TRANSFORM_FEEDBACK_BUFFER),
    AtomicCounter     = GLuint(gl::GL_ATOMIC_COUNTER_BUFFER),
};
JOSH3D_DEFINE_ENUM_EXTRAS(BufferTargetI,
    ShaderStorage,
    Uniform,
    TransformFeedback,
    AtomicCounter);

/*
NOTE: Since each texture target is a distinct type on-construction
for DSA style, we use the texutre target as a reflection enum.
*/
enum class TextureTarget : GLuint
{
    Texture1D        = GLuint(gl::GL_TEXTURE_1D),
    Texture1DArray   = GLuint(gl::GL_TEXTURE_1D_ARRAY),
    Texture2D        = GLuint(gl::GL_TEXTURE_2D),
    Texture2DArray   = GLuint(gl::GL_TEXTURE_2D_ARRAY),
    Texture2DMS      = GLuint(gl::GL_TEXTURE_2D_MULTISAMPLE),
    Texture2DMSArray = GLuint(gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY),
    Texture3D        = GLuint(gl::GL_TEXTURE_3D),
    Cubemap          = GLuint(gl::GL_TEXTURE_CUBE_MAP),
    CubemapArray     = GLuint(gl::GL_TEXTURE_CUBE_MAP_ARRAY),
    TextureRectangle = GLuint(gl::GL_TEXTURE_RECTANGLE),
    TextureBuffer    = GLuint(gl::GL_TEXTURE_BUFFER),
};
JOSH3D_DEFINE_ENUM_EXTRAS(TextureTarget,
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D,
    Cubemap,
    CubemapArray,
    TextureRectangle,
    TextureBuffer);

/*
TODO: Why not just call them "Compute", "Vertex", etc? Was it because of macros?
*/
enum class ShaderTarget : GLuint
{
    Compute        = GLuint(gl::GL_COMPUTE_SHADER),
    Vertex         = GLuint(gl::GL_VERTEX_SHADER),
    TessControl    = GLuint(gl::GL_TESS_CONTROL_SHADER),
    TessEvaluation = GLuint(gl::GL_TESS_EVALUATION_SHADER),
    Geometry       = GLuint(gl::GL_GEOMETRY_SHADER),
    Fragment       = GLuint(gl::GL_FRAGMENT_SHADER),
};
JOSH3D_DEFINE_ENUM_EXTRAS(ShaderTarget,
    Compute,
    Vertex,
    TessControl,
    TessEvaluation,
    Geometry,
    Fragment);

enum class QueryTarget : GLuint
{
    TimeElapsed                        = GLuint(gl::GL_TIME_ELAPSED),
    Timestamp                          = GLuint(gl::GL_TIMESTAMP),
    SamplesPassed                      = GLuint(gl::GL_SAMPLES_PASSED),
    AnySamplesPassed                   = GLuint(gl::GL_ANY_SAMPLES_PASSED),
    AnySamplesPassedConservative       = GLuint(gl::GL_ANY_SAMPLES_PASSED_CONSERVATIVE),
    PrimitivesGenerated                = GLuint(gl::GL_PRIMITIVES_GENERATED),
    VerticesSubmitted                  = GLuint(gl::GL_VERTICES_SUBMITTED),
    PrimitivesSubmitted                = GLuint(gl::GL_PRIMITIVES_SUBMITTED),
    VertexShaderInvocations            = GLuint(gl::GL_VERTEX_SHADER_INVOCATIONS),
    TessControlShaderPatches           = GLuint(gl::GL_TESS_CONTROL_SHADER_PATCHES),
    TessEvaluationShaderInvocations    = GLuint(gl::GL_TESS_EVALUATION_SHADER_INVOCATIONS),
    GeometryShaderInvocations          = GLuint(gl::GL_GEOMETRY_SHADER_INVOCATIONS),
    GeometryShaderPrimitivesEmitted    = GLuint(gl::GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED),
    ClippingInputPrimitives            = GLuint(gl::GL_CLIPPING_INPUT_PRIMITIVES),
    ClippingOutputPrimitives           = GLuint(gl::GL_CLIPPING_OUTPUT_PRIMITIVES),
    FragmentShaderInvocations          = GLuint(gl::GL_FRAGMENT_SHADER_INVOCATIONS),
    ComputeShaderInvocations           = GLuint(gl::GL_COMPUTE_SHADER_INVOCATIONS),
    TransformFeedbackPrimitivesWritten = GLuint(gl::GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN),
    TransformFeedbackOverflow          = GLuint(gl::GL_TRANSFORM_FEEDBACK_OVERFLOW),
    TransformFeedbackStreamOverflow    = GLuint(gl::GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW),
};
JOSH3D_DEFINE_ENUM_EXTRAS(QueryTarget,
    TimeElapsed,
    Timestamp,
    SamplesPassed,
    AnySamplesPassed,
    AnySamplesPassedConservative,
    PrimitivesGenerated,
    VerticesSubmitted,
    PrimitivesSubmitted,
    VertexShaderInvocations,
    TessControlShaderPatches,
    TessEvaluationShaderInvocations,
    GeometryShaderInvocations,
    GeometryShaderPrimitivesEmitted,
    ClippingInputPrimitives,
    ClippingOutputPrimitives,
    FragmentShaderInvocations,
    ComputeShaderInvocations,
    TransformFeedbackPrimitivesWritten,
    TransformFeedbackOverflow,
    TransformFeedbackStreamOverflow);

/*
This has no practical purpose other than "consistency".
*/
enum class FenceSyncTarget : GLuint
{
    GPUCommandsComplete = GLuint(gl::GL_SYNC_GPU_COMMANDS_COMPLETE),
};
JOSH3D_DEFINE_ENUM_EXTRAS(FenceSyncTarget, GPUCommandsComplete);

} // namespace josh
