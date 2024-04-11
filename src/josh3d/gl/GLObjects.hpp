#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLBuffers.hpp"      // IWYU pragma: export
#include "GLFenceSync.hpp"    // IWYU pragma: export
#include "GLFramebuffer.hpp"  // IWYU pragma: export
#include "GLMutability.hpp"
#include "GLProgram.hpp"      // IWYU pragma: export
#include "GLQueries.hpp"      // IWYU pragma: export
#include "GLSampler.hpp"      // IWYU pragma: export
#include "GLShaders.hpp"      // IWYU pragma: export
#include "GLTextures.hpp"     // IWYU pragma: export
#include "GLVertexArray.hpp"  // IWYU pragma: export
#include "GLUnique.hpp"       // IWYU pragma: export
#include "GLShared.hpp"       // IWYU pragma: export


namespace josh {


// TODO: These aliases could be moved to their respective object headers, as GLUnique
// does not depend on the concrete implementations of any of these objects.

#define JOSH3D_ALIAS_DSA_UNIQUE(Object) \
    using Unique##Object      = GLUnique<Raw##Object<GLMutable>>; \
    using UniqueConst##Object = GLUnique<Raw##Object<GLConst>>;

#define JOSH3D_ALIAS_DSA_SHARED(Object) \
    using Shared##Object      = GLShared<Raw##Object<GLMutable>>; \
    using SharedConst##Object = GLShared<Raw##Object<GLConst>>;

#define JOSH3D_ALIAS_DSA_OWNERS(Object) \
    JOSH3D_ALIAS_DSA_UNIQUE(Object)     \
    JOSH3D_ALIAS_DSA_SHARED(Object)


JOSH3D_ALIAS_DSA_OWNERS(UntypedBuffer)
template<trivially_copyable T> using UniqueBuffer      = GLUnique<RawBuffer<T, GLMutable>>;
template<trivially_copyable T> using UniqueConstBuffer = GLUnique<RawBuffer<T, GLConst>>;
template<trivially_copyable T> using SharedBuffer      = GLShared<RawBuffer<T, GLMutable>>;
template<trivially_copyable T> using SharedConstBuffer = GLShared<RawBuffer<T, GLConst>>;

JOSH3D_ALIAS_DSA_OWNERS(FenceSync)

JOSH3D_ALIAS_DSA_OWNERS(Framebuffer)

JOSH3D_ALIAS_DSA_OWNERS(Program)

JOSH3D_ALIAS_DSA_OWNERS(QueryTimeElapsed)
JOSH3D_ALIAS_DSA_OWNERS(QueryTimestamp)
JOSH3D_ALIAS_DSA_OWNERS(QuerySamplesPassed)
JOSH3D_ALIAS_DSA_OWNERS(QueryAnySamplesPassed)
JOSH3D_ALIAS_DSA_OWNERS(QueryAnySamplesPassedConservative)
JOSH3D_ALIAS_DSA_OWNERS(QueryPrimitivesGenerated)
JOSH3D_ALIAS_DSA_OWNERS(QueryVerticesSubmitted)
JOSH3D_ALIAS_DSA_OWNERS(QueryPrimitivesSubmitted)
JOSH3D_ALIAS_DSA_OWNERS(QueryVertexShaderInvocations)
JOSH3D_ALIAS_DSA_OWNERS(QueryTessControlShaderPatches)
JOSH3D_ALIAS_DSA_OWNERS(QueryTessEvaluationShaderInvocations)
JOSH3D_ALIAS_DSA_OWNERS(QueryGeometryShaderInvocations)
JOSH3D_ALIAS_DSA_OWNERS(QueryGeometryShaderPrimitivesEmitted)
JOSH3D_ALIAS_DSA_OWNERS(QueryClippingInputPrimitives)
JOSH3D_ALIAS_DSA_OWNERS(QueryClippingOutputPrimitives)
JOSH3D_ALIAS_DSA_OWNERS(QueryFragmentShaderInvocations)
JOSH3D_ALIAS_DSA_OWNERS(QueryComputeShaderInvocations)
JOSH3D_ALIAS_DSA_OWNERS(QueryTransformFeedbackPrimitivesWritten)
JOSH3D_ALIAS_DSA_OWNERS(QueryTransformFeedbackOverflow)
JOSH3D_ALIAS_DSA_OWNERS(QueryTransformFeedbackStreamOverflow)

JOSH3D_ALIAS_DSA_OWNERS(Sampler)

JOSH3D_ALIAS_DSA_OWNERS(ComputeShader)
JOSH3D_ALIAS_DSA_OWNERS(VertexShader)
JOSH3D_ALIAS_DSA_OWNERS(TessControlShader)
JOSH3D_ALIAS_DSA_OWNERS(TessEvaluationShader)
JOSH3D_ALIAS_DSA_OWNERS(GeometryShader)
JOSH3D_ALIAS_DSA_OWNERS(FragmentShader)

JOSH3D_ALIAS_DSA_OWNERS(Texture1D)
JOSH3D_ALIAS_DSA_OWNERS(Texture1DArray)
JOSH3D_ALIAS_DSA_OWNERS(Texture2D)
JOSH3D_ALIAS_DSA_OWNERS(Texture2DArray)
JOSH3D_ALIAS_DSA_OWNERS(Texture2DMS)
JOSH3D_ALIAS_DSA_OWNERS(Texture2DMSArray)
JOSH3D_ALIAS_DSA_OWNERS(Texture3D)
JOSH3D_ALIAS_DSA_OWNERS(Cubemap)
JOSH3D_ALIAS_DSA_OWNERS(CubemapArray)
JOSH3D_ALIAS_DSA_OWNERS(TextureRectangle)
JOSH3D_ALIAS_DSA_OWNERS(TextureBuffer)

JOSH3D_ALIAS_DSA_OWNERS(VertexArray)


} // namespace josh
