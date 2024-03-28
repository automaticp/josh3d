#pragma once
#include "CommonConcepts.hpp"    // IWYU pragma: keep
#include "GLBuffers.hpp"      // IWYU pragma: export
#include "GLFenceSync.hpp"    // IWYU pragma: export
#include "GLFramebuffer.hpp"  // IWYU pragma: export
#include "GLProgram.hpp"      // IWYU pragma: export
#include "GLQueries.hpp"      // IWYU pragma: export
#include "GLSampler.hpp"      // IWYU pragma: export
#include "GLShaders.hpp"      // IWYU pragma: export
#include "GLTextures.hpp"     // IWYU pragma: export
#include "GLVertexArray.hpp"  // IWYU pragma: export
#include "GLUnique.hpp"       // IWYU pragma: export



namespace josh {
namespace dsa {

// TODO: These aliases could be moved to their respective object headers, as GLUnique
// does not depend on the concrete implementations of any of these objects.

#define JOSH3D_ALIAS_DSA_UNIQUE(Object) \
    using Unique##Object = GLUnique<Raw##Object<GLMutable>>; // NOLINT(bugprone-macro-parentheses)


JOSH3D_ALIAS_DSA_UNIQUE(UntypedBuffer)
template<trivially_copyable T> using UniqueBuffer = GLUnique<RawBuffer<T, GLMutable>>;

JOSH3D_ALIAS_DSA_UNIQUE(FenceSync)

JOSH3D_ALIAS_DSA_UNIQUE(Framebuffer)

JOSH3D_ALIAS_DSA_UNIQUE(Program)

JOSH3D_ALIAS_DSA_UNIQUE(QueryTimeElapsed)
JOSH3D_ALIAS_DSA_UNIQUE(QueryTimestamp)
JOSH3D_ALIAS_DSA_UNIQUE(QuerySamplesPassed)
JOSH3D_ALIAS_DSA_UNIQUE(QueryAnySamplesPassed)
JOSH3D_ALIAS_DSA_UNIQUE(QueryAnySamplesPassedConservative)
JOSH3D_ALIAS_DSA_UNIQUE(QueryPrimitivesGenerated)
JOSH3D_ALIAS_DSA_UNIQUE(QueryVerticesSubmitted)
JOSH3D_ALIAS_DSA_UNIQUE(QueryPrimitivesSubmitted)
JOSH3D_ALIAS_DSA_UNIQUE(QueryVertexShaderInvocations)
JOSH3D_ALIAS_DSA_UNIQUE(QueryTessControlShaderPatches)
JOSH3D_ALIAS_DSA_UNIQUE(QueryTessEvaluationShaderInvocations)
JOSH3D_ALIAS_DSA_UNIQUE(QueryGeometryShaderInvocations)
JOSH3D_ALIAS_DSA_UNIQUE(QueryGeometryShaderPrimitivesEmitted)
JOSH3D_ALIAS_DSA_UNIQUE(QueryClippingInputPrimitives)
JOSH3D_ALIAS_DSA_UNIQUE(QueryClippingOutputPrimitives)
JOSH3D_ALIAS_DSA_UNIQUE(QueryFragmentShaderInvocations)
JOSH3D_ALIAS_DSA_UNIQUE(QueryComputeShaderInvocations)
JOSH3D_ALIAS_DSA_UNIQUE(QueryTransformFeedbackPrimitivesWritten)
JOSH3D_ALIAS_DSA_UNIQUE(QueryTransformFeedbackOverflow)
JOSH3D_ALIAS_DSA_UNIQUE(QueryTransformFeedbackStreamOverflow)

JOSH3D_ALIAS_DSA_UNIQUE(Sampler)

JOSH3D_ALIAS_DSA_UNIQUE(ComputeShader)
JOSH3D_ALIAS_DSA_UNIQUE(VertexShader)
JOSH3D_ALIAS_DSA_UNIQUE(TessControlShader)
JOSH3D_ALIAS_DSA_UNIQUE(TessEvaluationShader)
JOSH3D_ALIAS_DSA_UNIQUE(GeometryShader)
JOSH3D_ALIAS_DSA_UNIQUE(FragmentShader)

JOSH3D_ALIAS_DSA_UNIQUE(Texture1D)
JOSH3D_ALIAS_DSA_UNIQUE(Texture1DArray)
JOSH3D_ALIAS_DSA_UNIQUE(Texture2D)
JOSH3D_ALIAS_DSA_UNIQUE(Texture2DArray)
JOSH3D_ALIAS_DSA_UNIQUE(Texture2DMS)
JOSH3D_ALIAS_DSA_UNIQUE(Texture2DMSArray)
JOSH3D_ALIAS_DSA_UNIQUE(Texture3D)
JOSH3D_ALIAS_DSA_UNIQUE(Cubemap)
JOSH3D_ALIAS_DSA_UNIQUE(CubemapArray)
JOSH3D_ALIAS_DSA_UNIQUE(TextureRectangle)
JOSH3D_ALIAS_DSA_UNIQUE(TextureBuffer)

JOSH3D_ALIAS_DSA_UNIQUE(VertexArray)


} // namespace dsa
} // namespace josh
