#pragma once
#include "GLAPI.hpp"
#include "GLDSABuffers.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include "detail/ConditionalMixin.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <chrono>
#include <type_traits>



namespace josh::dsa {


enum class QueryTarget : GLuint {
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


namespace detail {


template<QueryTarget TargetV> struct query_result        { using type = GLuint64; };
template<> struct query_result<QueryTarget::TimeElapsed> { using type = std::chrono::nanoseconds; };
template<> struct query_result<QueryTarget::Timestamp>   { using type = std::chrono::nanoseconds; };

template<QueryTarget TargetV> struct is_query_indexed                               : std::false_type {};
template<> struct is_query_indexed<QueryTarget::PrimitivesGenerated>                : std::true_type {};
template<> struct is_query_indexed<QueryTarget::TransformFeedbackPrimitivesWritten> : std::true_type {};
template<> struct is_query_indexed<QueryTarget::TransformFeedbackStreamOverflow>    : std::true_type {};


template<typename CRTP, QueryTarget TargetV>
struct QueryDSAInterface_Common {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mutability = mutability_traits<CRTP>::mutability;
public:

    // Wraps `glGetQueryObject*` with `pname = GL_QUERY_RESULT_AVAILABLE`.
    bool is_available() const noexcept {
        GLboolean is_available;
        gl::glGetQueryObjectiv(self_id(), gl::GL_QUERY_RESULT_AVAILABLE, &is_available);
        return bool(is_available);
    }

    using result_type = query_result<TargetV>::type;

    // Wraps `glGetQueryObject*` with `pname = GL_QUERY_RESULT`.
    //
    // `GL_QUERY_RESULT_BUFFER` must be unbound during this call.
    //
    // `glGetQueryObject` implicitly flushes the GL pipeline so that any incomplete rendering
    // delimited by the occlusion query completes in finite time.
    //
    // If multiple queries are issued using the same query object id before calling `glGetQueryObject`,
    // the results of the most recent query will be returned. In this case, when issuing a new query,
    // the results of the previous query are discarded.
    auto result() const noexcept
        -> result_type
    {
        // TODO: Assert that no query buffer is bound.

        if constexpr (std::same_as<result_type, GLuint64>) {
            GLuint64 result;
            gl::glGetQueryObjectui64v(self_id(), gl::GL_QUERY_RESULT, &result);
            return result;
        } else if constexpr (std::same_as<result_type, std::chrono::nanoseconds>) {
            GLint64 result;
            gl::glGetQueryObjecti64v(self_id(), gl::GL_QUERY_RESULT, &result);
            return std::chrono::nanoseconds{ result };
        } else { static_assert(false); }
    }

    // Wraps `glGetQueryBufferObjectui64v` with `pname = GL_QUERY_RESULT`.
    //
    // Requires the buffer storage of at least of 64 bits to be available at `elem_offset`.
    // Will write a 64-bit unsigned integer at `elem_offset`.
    template<typename T>
    auto write_result_to_buffer(
        RawBuffer<GLMutable, T> buffer, GLintptr elem_offset) const noexcept
    {
        gl::glGetQueryBufferObjectui64v(
            self_id(), buffer.id(), gl::GL_QUERY_RESULT, elem_offset * sizeof(T)
        );
    }

};




template<typename CRTP>
struct QueryDSAInterface_CapabilityOfTimestamp {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mutability = mutability_traits<CRTP>::mutability;
public:
    // Wraps `glQueryCounter`.
    //
    // When `glQueryCounter` is called, the GL records the current time into the corresponding
    // query object. The time is recorded after all previous commands on
    // the GL client and server state and the framebuffer have been fully realized. When
    // the time is recorded, the query result for that object is marked available.
    //
    // See also `glapi::queries::current_time()`.
    void record_time() const noexcept
        requires gl_mutable<mutability>
    {
        gl::glQueryCounter(self_id(), enum_cast<GLenum>(QueryTarget::Timestamp));
    }

};


template<typename CRTP, QueryTarget TargetV>
struct QueryDSAInterface_CapabilityOfBeginEnd {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mutability = mutability_traits<CRTP>::mutability;
public:
    // Wraps `glBeginQuery`.
    void begin_query() const noexcept
        requires gl_mutable<mutability>
    {
        gl::glBeginQuery(enum_cast<GLenum>(TargetV), self_id());
    }

    // Wraps `glEndQuery`.
    void end_query() const noexcept
        requires gl_mutable<mutability>
    {
        gl::glEndQuery(enum_cast<GLenum>(TargetV));
    }

};


template<typename CRTP, QueryTarget TargetV>
struct QueryDSAInterface_CapabilityOfBeginEndIndexed {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mutability = mutability_traits<CRTP>::mutability;
public:

    // Wraps `glBeginQueryIndexed`.
    void begin_query_indexed(GLuint index) const noexcept
        requires gl_mutable<mutability>
    {
        gl::glBeginQueryIndexed(enum_cast<GLenum>(TargetV), index, self_id());
    }

    // Wraps `glEndQueryIndexed`.
    void end_query_indexed(GLuint index) const noexcept
        requires gl_mutable<mutability>
    {
        gl::glEndQueryIndexed(enum_cast<GLenum>(TargetV), index, self_id());
    }

};



template<typename CRTP, QueryTarget TargetV>
struct QueryDSAInterface_PrimaryCapability
    : QueryDSAInterface_CapabilityOfBeginEnd<CRTP, TargetV>
    , josh::detail::conditional_mixin_t<is_query_indexed<TargetV>::value,
        QueryDSAInterface_CapabilityOfBeginEndIndexed<CRTP, TargetV>>
{};

template<typename CRTP>
struct QueryDSAInterface_PrimaryCapability<CRTP, QueryTarget::Timestamp>
    : QueryDSAInterface_CapabilityOfTimestamp<CRTP>
{};




template<typename CRTP, QueryTarget TargetV>
struct QueryDSAInterface
    : QueryDSAInterface_Common<CRTP, TargetV>
    , QueryDSAInterface_PrimaryCapability<CRTP, TargetV>
{};


} // namespace detail


namespace detail {
using josh::detail::RawGLHandle;
} // namespace detail





#define JOSH3D_GENERATE_DSA_QUERY_CLASSES(Name)                                                                   \
    template<mutability_tag MutT = GLMutable>                                                                     \
    class RawQuery##Name                                                                                          \
        : public detail::RawGLHandle<MutT>                                                                        \
        , public detail::QueryDSAInterface<RawQuery##Name<MutT>, QueryTarget::Name>                               \
    {                                                                                                             \
    public:                                                                                                       \
        static constexpr GLKind      kind_type   = GLKind::Query;                                                 \
        static constexpr QueryTarget target_type = QueryTarget::Name;                                             \
        JOSH3D_MAGIC_CONSTRUCTORS_2(RawQuery##Name, mutability_traits<RawQuery##Name>, detail::RawGLHandle<MutT>) \
    };                                                                                                            \
    static_assert(sizeof(RawQuery##Name<GLMutable>) == sizeof(RawQuery##Name<GLConst>));


JOSH3D_GENERATE_DSA_QUERY_CLASSES(TimeElapsed)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(Timestamp)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(SamplesPassed)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(AnySamplesPassed)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(AnySamplesPassedConservative)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(PrimitivesGenerated)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(VerticesSubmitted)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(PrimitivesSubmitted)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(VertexShaderInvocations)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(TessControlShaderPatches)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(TessEvaluationShaderInvocations)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(GeometryShaderInvocations)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(GeometryShaderPrimitivesEmitted)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(ClippingInputPrimitives)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(ClippingOutputPrimitives)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(FragmentShaderInvocations)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(ComputeShaderInvocations)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(TransformFeedbackPrimitivesWritten)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(TransformFeedbackOverflow)
JOSH3D_GENERATE_DSA_QUERY_CLASSES(TransformFeedbackStreamOverflow)

#undef JOSH3D_GENERATE_DSA_QUERY_CLASSES




} // namespace josh::dsa
