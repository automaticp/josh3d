#pragma once
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLScalars.hpp"
#include "detail/GLAPIGet.hpp"


/*
Various implementation limits of the OpenGL API.
Specific limit queries are added here on a per-need basis.
*/
namespace josh {

enum class LimitB : GLuint
{
    PrimitiveRestartForPatchesSupported = GLuint(gl::GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED),
    ShaderCompiler                      = GLuint(gl::GL_SHADER_COMPILER),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitB,
    PrimitiveRestartForPatchesSupported,
    ShaderCompiler);

enum class LimitI : GLuint
{
    MaxClipDistances                             = GLuint(gl::GL_MAX_CLIP_DISTANCES),
    MaxCullDistances                             = GLuint(gl::GL_MAX_CULL_DISTANCES),
    MaxCombinedClipAndCullDistances              = GLuint(gl::GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES),
    SubpixelBits                                 = GLuint(gl::GL_SUBPIXEL_BITS),
    Max3dTextureSize                             = GLuint(gl::GL_MAX_3D_TEXTURE_SIZE),
    MaxTextureSize                               = GLuint(gl::GL_MAX_TEXTURE_SIZE),
    MaxArrayTextureLayers                        = GLuint(gl::GL_MAX_ARRAY_TEXTURE_LAYERS),
    MaxCubeMapTextureSize                        = GLuint(gl::GL_MAX_CUBE_MAP_TEXTURE_SIZE),
    MaxRenderbufferSize                          = GLuint(gl::GL_MAX_RENDERBUFFER_SIZE),
    MaxViewports                                 = GLuint(gl::GL_MAX_VIEWPORTS),
    ViewportSubpixelBits                         = GLuint(gl::GL_VIEWPORT_SUBPIXEL_BITS),
    MaxElementsIndices                           = GLuint(gl::GL_MAX_ELEMENTS_INDICES),
    MaxElementsVertices                          = GLuint(gl::GL_MAX_ELEMENTS_VERTICES),
    MaxVertexAttribRelativeOffset                = GLuint(gl::GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET),
    MaxVertexAttribBindings                      = GLuint(gl::GL_MAX_VERTEX_ATTRIB_BINDINGS),
    MaxVertexAttribStride                        = GLuint(gl::GL_MAX_VERTEX_ATTRIB_STRIDE),
    MaxTextureBufferSize                         = GLuint(gl::GL_MAX_TEXTURE_BUFFER_SIZE),
    MaxRectangleTextureSize                      = GLuint(gl::GL_MAX_RECTANGLE_TEXTURE_SIZE),
    MinMapBufferAlignment                        = GLuint(gl::GL_MIN_MAP_BUFFER_ALIGNMENT),
    TextureBufferOffsetAlignment                 = GLuint(gl::GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT),
    MaxVertexAttribs                             = GLuint(gl::GL_MAX_VERTEX_ATTRIBS),
    MaxVertexUniformComponents                   = GLuint(gl::GL_MAX_VERTEX_UNIFORM_COMPONENTS),
    MaxVertexUniformVectors                      = GLuint(gl::GL_MAX_VERTEX_UNIFORM_VECTORS),
    MaxVertexUniformBlocks                       = GLuint(gl::GL_MAX_VERTEX_UNIFORM_BLOCKS),
    MaxVertexOutputComponents                    = GLuint(gl::GL_MAX_VERTEX_OUTPUT_COMPONENTS),
    MaxVertexTextureImageUnits                   = GLuint(gl::GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS),
    MaxVertexAtomicCounterBuffers                = GLuint(gl::GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS),
    MaxVertexAtomicCounters                      = GLuint(gl::GL_MAX_VERTEX_ATOMIC_COUNTERS),
    MaxVertexShaderStorageBlocks                 = GLuint(gl::GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS),
    MaxTessGenLevel                              = GLuint(gl::GL_MAX_TESS_GEN_LEVEL),
    MaxPatchVertices                             = GLuint(gl::GL_MAX_PATCH_VERTICES),
    MaxTessControlUniformComponents              = GLuint(gl::GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS),
    MaxTessControlTextureImageUnits              = GLuint(gl::GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS),
    MaxTessControlOutputComponents               = GLuint(gl::GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS),
    MaxTessPatchComponents                       = GLuint(gl::GL_MAX_TESS_PATCH_COMPONENTS),
    MaxTessControlTotalOutputComponents          = GLuint(gl::GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS),
    MaxTessControlInputComponents                = GLuint(gl::GL_MAX_TESS_CONTROL_INPUT_COMPONENTS),
    MaxTessControlUniformBlocks                  = GLuint(gl::GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS),
    MaxTessControlAtomicCounterBuffers           = GLuint(gl::GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS),
    MaxTessControlAtomicCounters                 = GLuint(gl::GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS),
    MaxTessControlShaderStorageBlocks            = GLuint(gl::GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS),
    MaxTessEvaluationUniformComponents           = GLuint(gl::GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS),
    MaxTessEvaluationTextureImageUnits           = GLuint(gl::GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS),
    MaxTessEvaluationOutputComponents            = GLuint(gl::GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS),
    MaxTessEvaluationInputComponents             = GLuint(gl::GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS),
    MaxTessEvaluationUniformBlocks               = GLuint(gl::GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS),
    MaxTessEvaluationAtomicCounterBuffers        = GLuint(gl::GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS),
    MaxTessEvaluationAtomicCounters              = GLuint(gl::GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS),
    MaxTessEvaluationShaderStorageBlocks         = GLuint(gl::GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS),
    MaxGeometryUniformComponents                 = GLuint(gl::GL_MAX_GEOMETRY_UNIFORM_COMPONENTS),
    MaxGeometryUniformBlocks                     = GLuint(gl::GL_MAX_GEOMETRY_UNIFORM_BLOCKS),
    MaxGeometryInputComponents                   = GLuint(gl::GL_MAX_GEOMETRY_INPUT_COMPONENTS),
    MaxGeometryOutputComponents                  = GLuint(gl::GL_MAX_GEOMETRY_OUTPUT_COMPONENTS),
    MaxGeometryOutputVertices                    = GLuint(gl::GL_MAX_GEOMETRY_OUTPUT_VERTICES),
    MaxGeometryTotalOutputComponents             = GLuint(gl::GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS),
    MaxGeometryTextureImageUnits                 = GLuint(gl::GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS),
    MaxGeometryShaderInvocations                 = GLuint(gl::GL_MAX_GEOMETRY_SHADER_INVOCATIONS),
    MaxVertexStreams                             = GLuint(gl::GL_MAX_VERTEX_STREAMS),
    MaxGeometryAtomicCounterBuffers              = GLuint(gl::GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS),
    MaxGeometryAtomicCounters                    = GLuint(gl::GL_MAX_GEOMETRY_ATOMIC_COUNTERS),
    MaxGeometryShaderStorageBlocks               = GLuint(gl::GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS),
    MaxFragmentUniformComponents                 = GLuint(gl::GL_MAX_FRAGMENT_UNIFORM_COMPONENTS),
    MaxFragmentUniformVectors                    = GLuint(gl::GL_MAX_FRAGMENT_UNIFORM_VECTORS),
    MaxFragmentUniformBlocks                     = GLuint(gl::GL_MAX_FRAGMENT_UNIFORM_BLOCKS),
    MaxFragmentInputComponents                   = GLuint(gl::GL_MAX_FRAGMENT_INPUT_COMPONENTS),
    MaxFragmentTextureImageUnits                 = GLuint(gl::GL_MAX_TEXTURE_IMAGE_UNITS), // NOTE: Renamed for clarity.
    MinProgramTextureGatherOffset                = GLuint(gl::GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET),
    MaxProgramTextureGatherOffset                = GLuint(gl::GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET),
    MaxFragmentAtomicCounterBuffers              = GLuint(gl::GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS),
    MaxFragmentAtomicCounters                    = GLuint(gl::GL_MAX_FRAGMENT_ATOMIC_COUNTERS),
    MaxFragmentShaderStorageBlocks               = GLuint(gl::GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS),
    MaxComputeWorkGroupInvocations               = GLuint(gl::GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS),
    MaxComputeUniformBlocks                      = GLuint(gl::GL_MAX_COMPUTE_UNIFORM_BLOCKS),
    MaxComputeTextureImageUnits                  = GLuint(gl::GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS),
    MaxComputeAtomicCounterBuffers               = GLuint(gl::GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS),
    MaxComputeAtomicCounters                     = GLuint(gl::GL_MAX_COMPUTE_ATOMIC_COUNTERS),
    MaxComputeSharedMemorySize                   = GLuint(gl::GL_MAX_COMPUTE_SHARED_MEMORY_SIZE),
    MaxComputeUniformComponents                  = GLuint(gl::GL_MAX_COMPUTE_UNIFORM_COMPONENTS),
    MaxComputeImageUniforms                      = GLuint(gl::GL_MAX_COMPUTE_IMAGE_UNIFORMS),
    MaxCombinedComputeUniformComponents          = GLuint(gl::GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS),
    MaxComputeShaderStorageBlocks                = GLuint(gl::GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS),
    MinProgramTexelOffset                        = GLuint(gl::GL_MIN_PROGRAM_TEXEL_OFFSET),
    MaxProgramTexelOffset                        = GLuint(gl::GL_MAX_PROGRAM_TEXEL_OFFSET),
    MaxUniformBufferBindings                     = GLuint(gl::GL_MAX_UNIFORM_BUFFER_BINDINGS),
    MaxUniformBlockSize                          = GLuint(gl::GL_MAX_UNIFORM_BLOCK_SIZE),
    UniformBufferOffsetAlignment                 = GLuint(gl::GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT),
    MaxCombinedUniformBlocks                     = GLuint(gl::GL_MAX_COMBINED_UNIFORM_BLOCKS),
    MaxVaryingComponents                         = GLuint(gl::GL_MAX_VARYING_COMPONENTS),
    MaxVaryingVectors                            = GLuint(gl::GL_MAX_VARYING_VECTORS),
    MaxCombinedTextureImageUnits                 = GLuint(gl::GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS),
    MaxSubroutines                               = GLuint(gl::GL_MAX_SUBROUTINES),
    MaxSubroutineUniformLocations                = GLuint(gl::GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS),
    MaxUniformLocations                          = GLuint(gl::GL_MAX_UNIFORM_LOCATIONS),
    MaxAtomicCounterBufferBindings               = GLuint(gl::GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS),
    MaxAtomicCounterBufferSize                   = GLuint(gl::GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE),
    MaxCombinedAtomicCounterBuffers              = GLuint(gl::GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS),
    MaxCombinedAtomicCounters                    = GLuint(gl::GL_MAX_COMBINED_ATOMIC_COUNTERS),
    MaxShaderStorageBufferBindings               = GLuint(gl::GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS),
    MaxCombinedShaderStorageBlocks               = GLuint(gl::GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS),
    ShaderStorageBufferOffsetAlignment           = GLuint(gl::GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT),
    MaxImageUnits                                = GLuint(gl::GL_MAX_IMAGE_UNITS),
    MaxCombinedShaderOutputResources             = GLuint(gl::GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES),
    MaxImageSamples                              = GLuint(gl::GL_MAX_IMAGE_SAMPLES),
    MaxVertexImageUniforms                       = GLuint(gl::GL_MAX_VERTEX_IMAGE_UNIFORMS),
    MaxTessControlImageUniforms                  = GLuint(gl::GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS),
    MaxTessEvaluationImageUniforms               = GLuint(gl::GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS),
    MaxGeometryImageUniforms                     = GLuint(gl::GL_MAX_GEOMETRY_IMAGE_UNIFORMS),
    MaxFragmentImageUniforms                     = GLuint(gl::GL_MAX_FRAGMENT_IMAGE_UNIFORMS),
    MaxCombinedImageUniforms                     = GLuint(gl::GL_MAX_COMBINED_IMAGE_UNIFORMS),
    MaxCombinedVertexUniformComponents           = GLuint(gl::GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS),
    MaxCombinedGeometryUniformComponents         = GLuint(gl::GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS),
    MaxCombinedTessControlUniformComponents      = GLuint(gl::GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS),
    MaxCombinedTessEvaluationUniformComponents   = GLuint(gl::GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS),
    MaxCombinedFragmentUniformComponents         = GLuint(gl::GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS),
    MaxDebugMessageLength                        = GLuint(gl::GL_MAX_DEBUG_MESSAGE_LENGTH),
    MaxDebugLoggedMessages                       = GLuint(gl::GL_MAX_DEBUG_LOGGED_MESSAGES),
    MaxDebugGroupStackDepth                      = GLuint(gl::GL_MAX_DEBUG_GROUP_STACK_DEPTH),
    MaxLabelLength                               = GLuint(gl::GL_MAX_LABEL_LENGTH),
    MaxFramebufferWidth                          = GLuint(gl::GL_MAX_FRAMEBUFFER_WIDTH),
    MaxFramebufferHeight                         = GLuint(gl::GL_MAX_FRAMEBUFFER_HEIGHT),
    MaxFramebufferLayers                         = GLuint(gl::GL_MAX_FRAMEBUFFER_LAYERS),
    MaxFramebufferSamples                        = GLuint(gl::GL_MAX_FRAMEBUFFER_SAMPLES),
    MaxSampleMaskWords                           = GLuint(gl::GL_MAX_SAMPLE_MASK_WORDS),
    MaxSamples                                   = GLuint(gl::GL_MAX_SAMPLES),
    MaxColorTextureSamples                       = GLuint(gl::GL_MAX_COLOR_TEXTURE_SAMPLES),
    MaxDepthTextureSamples                       = GLuint(gl::GL_MAX_DEPTH_TEXTURE_SAMPLES),
    MaxIntegerSamples                            = GLuint(gl::GL_MAX_INTEGER_SAMPLES),
    FragmentInterpolationOffsetBits              = GLuint(gl::GL_FRAGMENT_INTERPOLATION_OFFSET_BITS),
    MaxDrawBuffers                               = GLuint(gl::GL_MAX_DRAW_BUFFERS),
    MaxDualSourceDrawBuffers                     = GLuint(gl::GL_MAX_DUAL_SOURCE_DRAW_BUFFERS),
    MaxColorAttachments                          = GLuint(gl::GL_MAX_COLOR_ATTACHMENTS),
    MaxTransformFeedbackInterleavedComponents    = GLuint(gl::GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS),
    MaxTransformFeedbackSeparateAttribs          = GLuint(gl::GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS),
    MaxTransformFeedbackSeparateComponents       = GLuint(gl::GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS),
    MaxTransformFeedbackBuffers                  = GLuint(gl::GL_MAX_TRANSFORM_FEEDBACK_BUFFERS),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitI,
    MaxClipDistances,
    MaxCullDistances,
    MaxCombinedClipAndCullDistances,
    SubpixelBits,
    Max3dTextureSize,
    MaxTextureSize,
    MaxArrayTextureLayers,
    MaxCubeMapTextureSize,
    MaxRenderbufferSize,
    MaxViewports,
    ViewportSubpixelBits,
    MaxElementsIndices,
    MaxElementsVertices,
    MaxVertexAttribRelativeOffset,
    MaxVertexAttribBindings,
    MaxVertexAttribStride,
    MaxTextureBufferSize,
    MaxRectangleTextureSize,
    MinMapBufferAlignment,
    TextureBufferOffsetAlignment,
    MaxVertexAttribs,
    MaxVertexUniformComponents,
    MaxVertexUniformVectors,
    MaxVertexUniformBlocks,
    MaxVertexOutputComponents,
    MaxVertexTextureImageUnits,
    MaxVertexAtomicCounterBuffers,
    MaxVertexAtomicCounters,
    MaxVertexShaderStorageBlocks,
    MaxTessGenLevel,
    MaxPatchVertices,
    MaxTessControlUniformComponents,
    MaxTessControlTextureImageUnits,
    MaxTessControlOutputComponents,
    MaxTessPatchComponents,
    MaxTessControlTotalOutputComponents,
    MaxTessControlInputComponents,
    MaxTessControlUniformBlocks,
    MaxTessControlAtomicCounterBuffers,
    MaxTessControlAtomicCounters,
    MaxTessControlShaderStorageBlocks,
    MaxTessEvaluationUniformComponents,
    MaxTessEvaluationTextureImageUnits,
    MaxTessEvaluationOutputComponents,
    MaxTessEvaluationInputComponents,
    MaxTessEvaluationUniformBlocks,
    MaxTessEvaluationAtomicCounterBuffers,
    MaxTessEvaluationAtomicCounters,
    MaxTessEvaluationShaderStorageBlocks,
    MaxGeometryUniformComponents,
    MaxGeometryUniformBlocks,
    MaxGeometryInputComponents,
    MaxGeometryOutputComponents,
    MaxGeometryOutputVertices,
    MaxGeometryTotalOutputComponents,
    MaxGeometryTextureImageUnits,
    MaxGeometryShaderInvocations,
    MaxVertexStreams,
    MaxGeometryAtomicCounterBuffers,
    MaxGeometryAtomicCounters,
    MaxGeometryShaderStorageBlocks,
    MaxFragmentUniformComponents,
    MaxFragmentUniformVectors,
    MaxFragmentUniformBlocks,
    MaxFragmentInputComponents,
    MaxFragmentTextureImageUnits,
    MinProgramTextureGatherOffset,
    MaxProgramTextureGatherOffset,
    MaxFragmentAtomicCounterBuffers,
    MaxFragmentAtomicCounters,
    MaxFragmentShaderStorageBlocks,
    MaxComputeWorkGroupInvocations,
    MaxComputeUniformBlocks,
    MaxComputeTextureImageUnits,
    MaxComputeAtomicCounterBuffers,
    MaxComputeAtomicCounters,
    MaxComputeSharedMemorySize,
    MaxComputeUniformComponents,
    MaxComputeImageUniforms,
    MaxCombinedComputeUniformComponents,
    MaxComputeShaderStorageBlocks,
    MinProgramTexelOffset,
    MaxProgramTexelOffset,
    MaxUniformBufferBindings,
    MaxUniformBlockSize,
    UniformBufferOffsetAlignment,
    MaxCombinedUniformBlocks,
    MaxVaryingComponents,
    MaxVaryingVectors,
    MaxCombinedTextureImageUnits,
    MaxSubroutines,
    MaxSubroutineUniformLocations,
    MaxUniformLocations,
    MaxAtomicCounterBufferBindings,
    MaxAtomicCounterBufferSize,
    MaxCombinedAtomicCounterBuffers,
    MaxCombinedAtomicCounters,
    MaxShaderStorageBufferBindings,
    MaxCombinedShaderStorageBlocks,
    ShaderStorageBufferOffsetAlignment,
    MaxImageUnits,
    MaxCombinedShaderOutputResources,
    MaxImageSamples,
    MaxVertexImageUniforms,
    MaxTessControlImageUniforms,
    MaxTessEvaluationImageUniforms,
    MaxGeometryImageUniforms,
    MaxFragmentImageUniforms,
    MaxCombinedImageUniforms,
    MaxCombinedVertexUniformComponents,
    MaxCombinedGeometryUniformComponents,
    MaxCombinedTessControlUniformComponents,
    MaxCombinedTessEvaluationUniformComponents,
    MaxCombinedFragmentUniformComponents,
    MaxDebugMessageLength,
    MaxDebugLoggedMessages,
    MaxDebugGroupStackDepth,
    MaxLabelLength,
    MaxFramebufferWidth,
    MaxFramebufferHeight,
    MaxFramebufferLayers,
    MaxFramebufferSamples,
    MaxSampleMaskWords,
    MaxSamples,
    MaxColorTextureSamples,
    MaxDepthTextureSamples,
    MaxIntegerSamples,
    FragmentInterpolationOffsetBits,
    MaxDrawBuffers,
    MaxDualSourceDrawBuffers,
    MaxColorAttachments,
    MaxTransformFeedbackInterleavedComponents,
    MaxTransformFeedbackSeparateAttribs,
    MaxTransformFeedbackSeparateComponents,
    MaxTransformFeedbackBuffers);

enum class LimitI64 : GLuint
{
    MaxElementIndex           = GLuint(gl::GL_MAX_ELEMENT_INDEX),
    MaxShaderStorageBlockSize = GLuint(gl::GL_MAX_SHADER_STORAGE_BLOCK_SIZE),
    MaxServerWaitTimeout      = GLuint(gl::GL_MAX_SERVER_WAIT_TIMEOUT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitI64,
    MaxElementIndex,
    MaxShaderStorageBlockSize,
    MaxServerWaitTimeout);

enum class Limit3I : GLuint
{
    MaxComputeWorkGroupCount = GLuint(gl::GL_MAX_COMPUTE_WORK_GROUP_COUNT),
    MaxComputeWorkGroupSize  = GLuint(gl::GL_MAX_COMPUTE_WORK_GROUP_SIZE),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Limit3I,
    MaxComputeWorkGroupCount,
    MaxComputeWorkGroupSize);

enum class LimitF : GLuint
{
    MaxTextureLodBias              = GLuint(gl::GL_MAX_TEXTURE_LOD_BIAS),
    MaxTextureMaxAnisotropy        = GLuint(gl::GL_MAX_TEXTURE_MAX_ANISOTROPY),
    PointSizeGranularity           = GLuint(gl::GL_POINT_SIZE_GRANULARITY),
    SmoothLineWidthGranularity     = GLuint(gl::GL_SMOOTH_LINE_WIDTH_GRANULARITY),
    MinFragmentInterpolationOffset = GLuint(gl::GL_MIN_FRAGMENT_INTERPOLATION_OFFSET),
    MaxFragmentInterpolationOffset = GLuint(gl::GL_MAX_FRAGMENT_INTERPOLATION_OFFSET),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitF,
    MaxTextureLodBias,
    MaxTextureMaxAnisotropy,
    PointSizeGranularity,
    SmoothLineWidthGranularity,
    MinFragmentInterpolationOffset,
    MaxFragmentInterpolationOffset);

enum class Limit2F : GLuint
{
    MaxViewportDims = GLuint(gl::GL_MAX_VIEWPORT_DIMS),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Limit2F,
    MaxViewportDims);

enum class LimitRF : GLuint
{
    ViewportBoundsRange   = GLuint(gl::GL_VIEWPORT_BOUNDS_RANGE),
    PointSizeRange        = GLuint(gl::GL_POINT_SIZE_RANGE),
    AliasedLineWidthRange = GLuint(gl::GL_ALIASED_LINE_WIDTH_RANGE),
    SmoothLineWidthRange  = GLuint(gl::GL_SMOOTH_LINE_WIDTH_RANGE),
};
JOSH3D_DEFINE_ENUM_EXTRAS(LimitRF,
    ViewportBoundsRange,
    PointSizeRange,
    AliasedLineWidthRange,
    SmoothLineWidthRange);


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

inline auto get_limit(LimitI64 limit)
    -> GLint64
{
    return detail::get_integer64(enum_cast<GLenum>(limit));
}

inline auto get_limit(Limit3I limit)
    -> std::array<GLint, 3>
{
    // NOTE: This works only for the MAX_WORKGROUP_[SIZE|COUNT].
    return {
        detail::get_integer_indexed(enum_cast<GLenum>(limit), 0),
        detail::get_integer_indexed(enum_cast<GLenum>(limit), 1),
        detail::get_integer_indexed(enum_cast<GLenum>(limit), 2),
    };
}

inline auto get_limit(LimitF limit)
    -> GLfloat
{
    return detail::get_float(enum_cast<GLenum>(limit));
}

inline auto get_limit(Limit2F limit)
    -> std::array<GLfloat, 2>
{
    // NOTE: This is dumb and should not be like this.
    // But for some reason this is queried with glGetFloatv()
    // even if the return values are Z+.
    return detail::get_floatv<2>(enum_cast<GLenum>(limit));
}

inline auto get_limit(LimitRF limit)
    -> RangeF
{
    auto [min, max] = detail::get_floatv<2>(enum_cast<GLenum>(limit));
    return { min, max };
}

} // namespace glapi
} // namespace josh

/*
Name                                             Enum  Type      Get Command     Minimum                 Description
-----------------------------------------------------------------------------------------------------------------------------------------
CONTEXT_RELEASE_BEHAVIOR                         -     E         GetIntegerv     See sec. 22.2           Flush behavior when context is released 22.2
MAX_CLIP_DISTANCES                               I     Z+        GetIntegerv     8                       Max. no. of user clipping planes 13.7
MAX_CULL_DISTANCES                               I     Z+        GetIntegerv     8                       Max. no. of user culling planes 13.7
MAX_COMBINED_CLIP_AND_CULL_DISTANCES             I     Z+        GetIntegerv     8                       Max. combined no. of user clipping 13.7
SUBPIXEL_BITS                                    I     Z+        GetIntegerv     4                       No. of bits of subpixel precision in screen xw and yw 14
MAX_ELEMENT_INDEX                                I64   Z+        GetInteger64v   2^32 − 1                 Max. element index 10.4
PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED          B     B         GetBooleanv     –                       Primitive restart support for PATCHES 10.3.6
MAX_3D_TEXTURE_SIZE                              I     Z+        GetIntegerv     2048                    Max. 3D texture image dimension 8.5
MAX_TEXTURE_SIZE                                 I     Z+        GetIntegerv     16384                   Max. 2D/1D texture image dimen-sion 8.5
MAX_ARRAY_TEXTURE_LAYERS                         I     Z+        GetIntegerv     2048                    Max. no. of layers for texture arrays 8.5
MAX_TEXTURE_LOD_BIAS                             F     R+        GetFloatv       2.0                     Max. absolute texture level-of-detail bias 8.14
MAX_CUBE_MAP_TEXTURE_SIZE                        I     Z+        GetIntegerv     16384                   Max. cube map texture image di-mension 8.5
MAX_RENDERBUFFER_SIZE                            I     Z+        GetIntegerv     16384                   Max. width and height of render-buffers 9.2.4
MAX_TEXTURE_MAX_ANISOTROPY                       F     R         GetFloatv       16.0                    Limit of maximum degree of anisotropy 8.14
MAX_VIEWPORT_DIMS                                2I    2 x Z+    GetFloatv       See sec. 13.8.1         Max. viewport dimensions 13.8.1
MAX_VIEWPORTS                                    I     Z+        GetIntegerv     16                      Max. no. of active view-ports 13.8.1
VIEWPORT_SUBPIXEL_BITS                           I     Z+        GetIntegerv     0                       No. of bits of subpixel precision for viewport bounds 13.8.1
VIEWPORT_BOUNDS_RANGE                            RF    2 x R     GetFloatv       †                       Viewport bounds range [min, max] † (at least[−32768, 32767]) 13.8.1
LAYER_PROVOKING_VERTEX                           -     E         GetIntegerv     See sec. 11.3.4         Vertex convention followed by gl_Layer 11.3.4
VIEWPORT_INDEX_PROVOKING_VERTEX                  -     E         GetIntegerv     See sec. 11.3.4         Vertex convention followed by gl_ViewportIndex 11.3.4
POINT_SIZE_RANGE                                 RF    2 x R+    GetFloatv       1,1                     Range (lo to hi) of point sprite sizes 14.4
POINT_SIZE_GRANULARITY                           F     R+        GetFloatv       –                       Point sprite size granularity 14.4
ALIASED_LINE_WIDTH_RANGE                         RF    2 x R+    GetFloatv       1,1                     Range (lo to hi) of aliased line widths 14.5
SMOOTH_LINE_WIDTH_RANGE                          RF    2 x R+    GetFloatv       1,1                     Range (lo to hi) of antialiased line widths 14.5
SMOOTH_LINE_WIDTH_GRANULARITY                    F     R+        GetFloatv       –                       Antialiased line width granularity 14.5
MAX_ELEMENTS_INDICES                             I     Z+        GetIntegerv     –                       Recommended max no. of DrawRangeElements indices 10.3
MAX_ELEMENTS_VERTICES                            I     Z+        GetIntegerv     –                       Recommended max no. of DrawRangeElements vertices 10.3
MAX_VERTEX_ATTRIB_RELATIVE_OFFSET                I     Z         GetIntegerv     2047                    Max. offset added to vertex buffer binding offset 10.3
MAX_VERTEX_ATTRIB_BINDINGS                       I     Z16*      GetIntegerv     16                      Max. no. of vertex buffers 10.3
MAX_VERTEX_ATTRIB_STRIDE                         I     Z         GetIntegerv     2048                    Max. vertex attribute stride 10.3
NUM_COMPRESSED_TEXTURE_FORMATS                   -     Z+        GetIntegerv     18                      No. of compressed texture formats 8.7
COMPRESSED_TEXTURE_FORMATS                       -     18 * xZ+  GetIntegerv     -                       Enumerated compressed texture formats 8.7
MAX_TEXTURE_BUFFER_SIZE                          I     Z+        GetIntegerv     65536                   No. of addressable texels for buffer textures 8.9
MAX_RECTANGLE_TEXTURE_SIZE                       I     Z+        GetIntegerv     16384                   Max. width & height of rectangle textures 8.5
NUM_PROGRAM_BINARY_FORMATS                       -     Z+        GetIntegerv     0                       No. of program binary formats 7.5
PROGRAM_BINARY_FORMATS                           -     0 * xZ+   GetIntegerv     N/A                     Enumerated program binary formats 7.5
NUM_SHADER_BINARY_FORMATS                        -     Z+        GetIntegerv     0                       No. of shader binary formats 7.2
SHADER_BINARY_FORMATS                            -     0 * xZ+   GetIntegerv     -                       Enumerated shader binary formats 7.2
SHADER_COMPILER                                  B     B         GetBooleanv     TRUE                    Shader compiler supported 7
MIN_MAP_BUFFER_ALIGNMENT                         I     Z+        GetIntegerv     64                      Min byte alignment of pointers returned by Map*Buffer 6.3
TEXTURE_BUFFER_OFFSET_ALIGNMENT                  I     Z+        GetIntegerv     256†                    Min. required alignment for texture buffer offsets
MAX_VERTEX_ATTRIBS                               I     Z+        GetIntegerv     16                      No. of active vertex attributes 10.2
MAX_VERTEX_UNIFORM_COMPONENTS                    I     Z+        GetIntegerv     1024                    No. of components for vertex shader uniform variables 7.6
MAX_VERTEX_UNIFORM_VECTORS                       I     Z+        GetIntegerv     256                     No. of vectors for vertex shader uniform variables 7.6
MAX_VERTEX_UNIFORM_BLOCKS                        I     Z+        GetIntegerv     14*                     Max. no. of vertex uniform buffers per program 7.6.2
MAX_VERTEX_OUTPUT_COMPONENTS                     I     Z+        GetIntegerv     64                      Max. no. of components of outputs written by a vertex shader 11.1.2.1
MAX_VERTEX_TEXTURE_IMAGE_UNITS                   I     Z+        GetIntegerv     16                      No. of texture image units accessible by a vertex shader 11.1.3.5
MAX_VERTEX_ATOMIC_COUNTER_BUFFERS                I     Z+        GetIntegerv     0                       No. of atomic counter buffers accessed by a vertex shader 7.7
MAX_VERTEX_ATOMIC_COUNTERS                       I     Z+        GetIntegerv     0                       No. of atomic counters accessed by a vertex shader 11.1.3.6
MAX_VERTEX_SHADER_STORAGE_BLOCKS                 I     Z+        GetIntegerv     0                       No. of shader storage blocks accessed by a vertex shader 7.8
MAX_TESS_GEN_LEVEL                               I     Z+        GetIntegerv     64                      Max. level supported by tess. primitive generator 11.2.2
MAX_PATCH_VERTICES                               I     Z+        GetIntegerv     32                      Max. patch size 10.1
MAX_TESS_CONTROL_UNIFORM_COMPONENTS              I     Z+        GetIntegerv     1024                    No. of words for tess. control shader (TCS) uniforms 11.2.1.1
MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS             I     Z+        GetIntegerv     16                      No. of tex. image units for TCS 11.1.3
MAX_TESS_CONTROL_OUTPUT_COMPONENTS               I     Z+        GetIntegerv     128                     No. components for TCS per-vertex outputs 11.2.1.2
MAX_TESS_PATCH_COMPONENTS                        I     Z+        GetIntegerv     120                     No. components for TCS per-patch outputs 11.2.1.2
MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS         I     Z+        GetIntegerv     4096                    Total no. components for TCS outputs 11.2.1.2
MAX_TESS_CONTROL_INPUT_COMPONENTS                I     Z+        GetIntegerv     128                     No. components for TCS per-vertex inputs 11.2.1.2
MAX_TESS_CONTROL_UNIFORM_BLOCKS                  I     Z+        GetIntegerv     14*                     No. of supported uniform blocks for TCS 7.6.2
MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS          I     Z+        GetIntegerv     0                       No. of atomic counter (AC) buffers accessed by a TCS 7.7
MAX_TESS_CONTROL_ATOMIC_COUNTERS                 I     Z+        GetIntegerv     0                       No. of ACs accessed by a TCS 7.7
MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS           I     Z+        GetIntegerv     0                       No. of shader storage blocks accessed by a tess. control shader 7.8
MAX_TESS_EVALUATION_UNIFORM_COMPONENTS           I     Z+        GetIntegerv     1024                    No. of words for tess. evaluation shader (TES) uniforms 11.2.3.1
MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS          I     Z+        GetIntegerv     16                      No. of tex. image units for TES 11.1.3
MAX_TESS_EVALUATION_OUTPUT_COMPONENTS            I     Z+        GetIntegerv     128                     No. components for TES per-vertex outputs 11.2.3.2
MAX_TESS_EVALUATION_INPUT_COMPONENTS             I     Z+        GetIntegerv     128                     No. components for TES per-vertex inputs 11.2.3.2
MAX_TESS_EVALUATION_UNIFORM_BLOCKS               I     Z+        GetIntegerv     14*                     No. of supported uniform blocks for TES 7.6.2
MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS       I     Z+        GetIntegerv     0                       No. of AC buffers accessed by a TES 11.1.3.6
MAX_TESS_EVALUATION_ATOMIC_COUNTERS              I     Z+        GetIntegerv     0                       No. of ACs accessed by a TES 11.1.3.6
MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS        I     Z+        GetIntegerv     0                       No. of shader storage blocks accessed by a tess. evaluation shader 7.8
MAX_GEOMETRY_UNIFORM_COMPONENTS                  I     Z+        GetIntegerv     1024                    No. of components for geometry shader (GS) uniform variables 11.3.3
MAX_GEOMETRY_UNIFORM_BLOCKS                      I     Z+        GetIntegerv     14*                     Max. no. of GS uniform buffers per program 7.6.2
MAX_GEOMETRY_INPUT_COMPONENTS                    I     Z+        GetIntegerv     64                      Max. no. of components of inputs read by a GS 11.3.4.4
MAX_GEOMETRY_OUTPUT_COMPONENTS                   I     Z+        GetIntegerv     128                     Max. no. of components of outputs written by a GS 11.3.4.5
MAX_GEOMETRY_OUTPUT_VERTICES                     I     Z+        GetIntegerv     256                     Max. no. of vertices that any GS can emit 11.3.4
MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS             I     Z+        GetIntegerv     1024                    Max. no. of total components (all vertices) of active outputs that a GS can emit 11.3.4
MAX_GEOMETRY_TEXTURE_IMAGE_UNITS                 I     Z+        GetIntegerv     16                      No. of texture image units accessible by a GS 11.3.4
MAX_GEOMETRY_SHADER_INVOCATIONS                  I     Z+        GetIntegerv     32                      Max. supported GS invocation count 11.3.4.2
MAX_VERTEX_STREAMS                               I     Z+        GetIntegerv     4                       Total no. of vertex streams 11.3.4.2
MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS              I     Z+        GetIntegerv     0                       No. of atomic counter buffers accessed by a GS 7.7
MAX_GEOMETRY_ATOMIC_COUNTERS                     I     Z+        GetIntegerv     0                       No. of atomic counters accessed by a GS 11.1.3.6
MAX_GEOMETRY_SHADER_STORAGE_BLOCKS               I     Z+        GetIntegerv     0                       No. of shader storage blocks accessed by a GS 7.8
MAX_FRAGMENT_UNIFORM_COMPONENTS                  I     Z+        GetIntegerv     1024                    No. of components for fragment shader (FS) uniform variables 15.1
MAX_FRAGMENT_UNIFORM_VECTORS                     I     Z+        GetIntegerv     256                     No. of vectors for FS uniform variables 15.1
MAX_FRAGMENT_UNIFORM_BLOCKS                      I     Z+        GetIntegerv     14*                     Max. no. of FS uniform buffers per program 7.6.2
MAX_FRAGMENT_INPUT_COMPONENTS                    I     Z+        GetIntegerv     128                     Max. no. of components of inputs read by a FS 15.2.2
MAX_TEXTURE_IMAGE_UNITS                          I     Z+        GetIntegerv     16                      No. of texture image units accessible by a FS 11.1.3.5
MIN_PROGRAM_TEXTURE_GATHER_OFFSET                I     Z         GetIntegerv     -8                      Min texel offset for textureGather 8.14.1
MAX_PROGRAM_TEXTURE_GATHER_OFFSET                I     Z+        GetIntegerv     7                       Max. texel offset for textureGather 8.14.1
MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS              I     Z+        GetIntegerv     1                       No. of atomic counter buffers accessed by a FS 7.7
MAX_FRAGMENT_ATOMIC_COUNTERS                     I     Z+        GetIntegerv     8                       No. of atomic counters accessed by a FS 11.1.3.6
MAX_FRAGMENT_SHADER_STORAGE_BLOCKS               I     Z+        GetIntegerv     8                       No. of shader storage blocks accessed by a FS 7.8
MAX_COMPUTE_WORK_GROUP_COUNT                     3I    3 x Z+    GetIntegeri     v 65535                 Max. no. of workgroups (WG) that may be dispatched by a single dispatch command (per dimension) 19
MAX_COMPUTE_WORK_GROUP_SIZE                      3I    3 x Z+    GetIntegeri     v 1024 (x, y), 64 (z)   Max. local size of a compute WG (per dimension) 19
MAX_COMPUTE_WORK_GROUP_INVOCATIONS               I     Z+        GetIntegerv     1024                    Max. total compute shader (CS) invocations in a single local WG 19
MAX_COMPUTE_UNIFORM_BLOCKS                       I     Z+        GetIntegerv     14*                     Max. no. of uniform blocks per compute program 7.6.2
MAX_COMPUTE_TEXTURE_IMAGE_UNITS                  I     Z+        GetIntegerv     16                      Max. no. of texture image units accessible by a CS 11.1.3.5
MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS               I     Z+        GetIntegerv     8                       No. of atomic counter buffers accessed by a CS 7.7
MAX_COMPUTE_ATOMIC_COUNTERS                      I     Z+        GetIntegerv     8                       No. of atomic counters accessed by a CS 11.1.3.6
MAX_COMPUTE_SHARED_MEMORY_SIZE                   I     Z+        GetIntegerv     32768                   Max. total storage size of all variables declared as shared in all CSs linked into a single program object 19.1
MAX_COMPUTE_UNIFORM_COMPONENTS                   I     Z+        GetIntegerv     1024                    No. of components for CS uniform variables 19.1
MAX_COMPUTE_IMAGE_UNIFORMS                       I     Z+        GetIntegerv     8                       No. of image variables in compute shaders 11.1.3
MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS          I     Z+        GetIntegerv     *                       No. of words for compute shader uniform variables in all uniform blocks, including the default 19.1
MAX_COMPUTE_SHADER_STORAGE_BLOCKS                I     Z+        GetIntegerv     8                       No. of shader storage blocks accessed by a compute shader 7.8
MIN_PROGRAM_TEXEL_OFFSET                         I     Z         GetIntegerv     -8                      Min texel offset allowed in lookup 11.1.3.5
MAX_PROGRAM_TEXEL_OFFSET                         I     Z         GetIntegerv     7                       Max. texel offset allowed in lookup 11.1.3.5
MAX_UNIFORM_BUFFER_BINDINGS                      I     Z+        GetIntegerv     84                      Max. no. of uniform buffer binding points on the context 7.6.2
MAX_UNIFORM_BLOCK_SIZE                           I     Z+        GetIntegerv     16384                   Max. size in basic machine units of a uniform block 7.6.2
UNIFORM_BUFFER_OFFSET_ALIGNMENT                  I     Z+        GetIntegerv     256†                    Min. required alignment for uniform buffer sizes and offsets 7.6.2
MAX_COMBINED_UNIFORM_BLOCKS                      I     Z+        GetIntegerv     70*                     Max. no. of uniform buffers per program 7.6.2
MAX_VARYING_COMPONENTS                           I     Z+        GetIntegerv     60                      No. of components for output variables 11.1.2.1
MAX_VARYING_VECTORS                              I     Z+        GetIntegerv     15                      No. of vectors for output variables 11.1.2.1
MAX_COMBINED_TEXTURE_IMAGE_UNITS                 I     Z+        GetIntegerv     80                      Total no. of texture units accessible by the GL 11.1.3.5
MAX_SUBROUTINES                                  I     Z+        GetIntegerv     256                     Max. no. of subroutines per shader stage 7.10
MAX_SUBROUTINE_UNIFORM_LOCATIONS                 I     Z+        GetIntegerv     1024                    Max. no. of subroutine uniform locations per stage 7.10
MAX_UNIFORM_LOCATIONS                            I     Z+        GetIntegerv     1024                    Max. no. of user-assignable uniform locations 7.6
MAX_ATOMIC_COUNTER_BUFFER_BINDINGS               I     Z+        GetIntegerv     1                       Max. no. of atomic counter buffer bindings 6.8
MAX_ATOMIC_COUNTER_BUFFER_SIZE                   I     Z+        GetIntegerv     32                      Max. size in basic machine units of an atomic counter buffer 7.7
MAX_COMBINED_ATOMIC_COUNTER_BUFFERS              I     Z+        GetIntegerv     1                       Max. no. of atomic counter buffers per program 7.7
MAX_COMBINED_ATOMIC_COUNTERS                     I     Z+        GetIntegerv     8                       Max. no. of atomic counter uniforms per program 11.1.3.6
MAX_SHADER_STORAGE_BUFFER_BINDINGS               I     Z+        GetIntegerv     8                       Max. no. of shader storage buffer bindings in the context 7.8
MAX_SHADER_STORAGE_BLOCK_SIZE                    I64   Z+        GetInteger64v   227                     Max. size in basic machine units of a shader storage block 7.8
MAX_COMBINED_SHADER_STORAGE_BLOCKS               I     Z+        GetIntegerv     8                       No. of shader storage blocks accessed by a program 7.8
SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT           I     Z+        GetIntegerv     256†                    Min. required alignment for shader storage buffer binding offsets 7.8
MAX_IMAGE_UNITS                                  I     Z+        GetIntegerv     8                       No. of units for image load/store/atom 8.26
MAX_COMBINED_SHADER_OUTPUT_RESOURCES             I     Z+        GetIntegerv     8                       Limit on active image units + shader storage blocks + fragment outputs 8.26
MAX_IMAGE_SAMPLES                                I     Z+        GetIntegerv     0                       Max. allowed samples for a texture level bound to an image unit 8.26
MAX_VERTEX_IMAGE_UNIFORMS                        I     Z+        GetIntegerv     0                       No. of image variables in vertex shaders 11.1.3.7
MAX_TESS_CONTROL_IMAGE_UNIFORMS                  I     Z+        GetIntegerv     0                       No. of image variables in tess. control shaders 11.1.3.7
MAX_TESS_EVALUATION_IMAGE_UNIFORMS               I     Z+        GetIntegerv     0                       No. of image variables in tess. eval. shaders 11.1.3.7
MAX_GEOMETRY_IMAGE_UNIFORMS                      I     Z+        GetIntegerv     0                       No. of image variables in geometry shaders 11.1.3.7
MAX_FRAGMENT_IMAGE_UNIFORMS                      I     Z+        GetIntegerv     8                       No. of image variables in fragment shaders 11.1.3.7
MAX_COMBINED_IMAGE_UNIFORMS                      I     Z+        GetIntegerv     8                       No. of image variables in all shaders 11.1.3.7
MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS           I     Z+        GetIntegerv     †                       No. of words for vertex shader uniform variables in all uniform blocks (including default) 7.6.2
MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS         I     Z+        GetIntegerv     †                       No. of words for geometry shader uniform variables in all uniform blocks (including default) 7.6.2
MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS     I     Z+        GetIntegerv     †                       No. of words for TCS uniform variables in all uniform blocks (including default) 11.2.1.1
MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS  I     Z+        GetIntegerv     †                       No. of words for TES uniform variables in all uniform blocks (including default) 11.2.3.1
MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS         I     Z+        GetIntegerv     †                       No. of words for fragment shader uniform variables in all uniform blocks (including default) 7.6.2
MAX_DEBUG_MESSAGE_LENGTH                         I     Z+        GetIntegerv     1                       The max length of a debug message string, including its null terminator 20.1
MAX_DEBUG_LOGGED_MESSAGES                        I     Z+        GetIntegerv     1                       The max no. of messages stored in the debug message log 20.3
MAX_DEBUG_GROUP_STACK_DEPTH                      I     Z+        GetIntegerv     64                      Max. group stack depth 20.6
MAX_LABEL_LENGTH                                 I     Z+        GetIntegerv     256                     Max. length of a label string 20.7
MAX_FRAMEBUFFER_WIDTH                            I     Z+        GetIntegerv     16384                   Max. width for framebuffer object 9.2.1
MAX_FRAMEBUFFER_HEIGHT                           I     Z+        GetIntegerv     16384                   Max. height for framebuffer object 9.2.1
MAX_FRAMEBUFFER_LAYERS                           I     Z+        GetIntegerv     2048                    Max. layer count for layered framebuffer object 9.2.1
MAX_FRAMEBUFFER_SAMPLES                          I     Z+        GetIntegerv     4                       Max. sample count for framebuffer object 9.2.1
MAX_SAMPLE_MASK_WORDS                            I     Z+        GetIntegerv     1                       Max. no. of sample mask words 14.9.3
MAX_SAMPLES                                      I     Z+        GetIntegerv     4                       Max. no. of samples supported for multisampling† 9.2.4
MAX_COLOR_TEXTURE_SAMPLES                        I     Z+        GetIntegerv     1                       Max. no. of samples supported for all color formats in a multisample texture† 22.3
MAX_DEPTH_TEXTURE_SAMPLES                        I     Z+        GetIntegerv     1                       Max. no. of samples supported for all depth/stencil formats in a multisample texture† 22.3
MAX_INTEGER_SAMPLES                              I     Z+        GetIntegerv     1                       Max. no. of samples supported for all integer format multisample buffers† 22.3
QUERY_COUNTER_BITS                               -     n x Z+    GetQueryiv      See sec. 4.2.3          Asynchronous query counter bits 4.2.3
MAX_SERVER_WAIT_TIMEOUT                          I64   Z+        GetInteger64v   0                       Max. WaitSync timeout interval 4.1.1
MIN_FRAGMENT_INTERPOLATION_OFFSET                F     R         GetFloatv       -0.5                    Furthest negative offset for interpolateAtOffset 15.1
MAX_FRAGMENT_INTERPOLATION_OFFSET                F     R         GetFloatv       +0.5 - 1 ULP†           Furthest positive offset for interpolateAtOffset 15.1
FRAGMENT_INTERPOLATION_OFFSET_BITS               I     Z+        GetIntegerv     4                       Subpixel bits for interpolateAtOffset 15.1
MAX_DRAW_BUFFERS                                 I     Z+        GetIntegerv     8                       Max. no. of active draw buffers 17.4.1
MAX_DUAL_SOURCE_DRAW_BUFFERS                     I     Z+        GetIntegerv     1                       Max. no. of active draw buffers when using dualsource blending 17.3.6
MAX_COLOR_ATTACHMENTS                            I     Z+        GetIntegerv     8                       Max. no. of FBO attachment points for color buffers 9.2.7
MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS    I     Z+        GetIntegerv     64                      Max. no. of components to write to a single buffer in interleaved mode 13.3
MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS          I     Z+        GetIntegerv     4                       Max. no. of separate attributes or outputs that can be captured in transform feedback 13.3
MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS       I     Z+        GetIntegerv     4                       Max. no. of components per attribute or output in separate mode 13.3
MAX_TRANSFORM_FEEDBACK_BUFFERS                   I     Z+        GetIntegerv     4                       Max. no. of buffer objs to write with transform feedback 13.3
*/

