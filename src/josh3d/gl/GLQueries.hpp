#pragma once
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPITargets.hpp"
#include "GLBuffers.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/MixinHeaderMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include "detail/ConditionalMixin.hpp"
#include "detail/StaticAssertFalseMacro.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <chrono>
#include <type_traits>


namespace josh {
namespace detail {
namespace query_api {


template<QueryTarget TargetV> struct query_result        { using type = GLuint64; };
template<> struct query_result<QueryTarget::TimeElapsed> { using type = std::chrono::nanoseconds; };
template<> struct query_result<QueryTarget::Timestamp>   { using type = std::chrono::nanoseconds; };

template<QueryTarget TargetV> struct is_query_indexed                               : std::false_type {};
template<> struct is_query_indexed<QueryTarget::PrimitivesGenerated>                : std::true_type {};
template<> struct is_query_indexed<QueryTarget::TransformFeedbackPrimitivesWritten> : std::true_type {};
template<> struct is_query_indexed<QueryTarget::TransformFeedbackStreamOverflow>    : std::true_type {};


template<typename CRTP, QueryTarget TargetV>
struct Common
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glGetQueryObject*` with `pname = GL_QUERY_RESULT_AVAILABLE`.
    auto is_available() const -> bool
    {
        GLboolean is_available;
        gl::glGetQueryObjectiv(self_id(), gl::GL_QUERY_RESULT_AVAILABLE, &is_available);
        return bool(is_available);
    }

    using query_result_type = query_result<TargetV>::type;

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
    auto result() const -> query_result_type
    {
        assert(glapi::get_bound_id(Binding::QueryBuffer) == 0);
        if constexpr (std::same_as<query_result_type, GLuint64>)
        {
            GLuint64 result;
            gl::glGetQueryObjectui64v(self_id(), gl::GL_QUERY_RESULT, &result);
            return result;
        }
        else if constexpr (std::same_as<query_result_type, std::chrono::nanoseconds>)
        {
            GLint64 result;
            gl::glGetQueryObjecti64v(self_id(), gl::GL_QUERY_RESULT, &result);
            return std::chrono::nanoseconds{ result };
        }
        else
        { JOSH3D_STATIC_ASSERT_FALSE(query_result_type); }
    }

    // Wraps `glGetQueryBufferObjectui64v` with `pname = GL_QUERY_RESULT`.
    //
    // Requires the buffer storage of at least of 64 bits to be available at `elem_offset`.
    // Will write a 64-bit unsigned integer at `elem_offset`.
    template<typename T>
    void write_result_to_buffer(RawBuffer<GLMutable, T> buffer, GLintptr elem_offset) const
    {
        gl::glGetQueryBufferObjectui64v(
            self_id(), buffer.id(), gl::GL_QUERY_RESULT, elem_offset * sizeof(T));
    }
};


template<typename CRTP>
struct CapabilityOfTimestamp
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glQueryCounter`.
    //
    // When `glQueryCounter` is called, the GL records the current time into the corresponding
    // query object. The time is recorded after all previous commands on
    // the GL client and server state and the framebuffer have been fully realized. When
    // the time is recorded, the query result for that object is marked available.
    //
    // See also `glapi::get_current_time()`.
    void record_time() const requires mt::is_mutable
    {
        gl::glQueryCounter(self_id(), enum_cast<GLenum>(QueryTarget::Timestamp));
    }
};


template<typename CRTP, QueryTarget TargetV>
struct CapabilityOfBeginEnd
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glBeginQuery`.
    void begin_query() const requires mt::is_mutable
    {
        gl::glBeginQuery(enum_cast<GLenum>(TargetV), self_id());
    }

    // Wraps `glEndQuery`.
    void end_query() const requires mt::is_mutable
    {
        gl::glEndQuery(enum_cast<GLenum>(TargetV));
    }
};


template<typename CRTP, QueryTarget TargetV>
struct CapabilityOfBeginEndIndexed
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glBeginQueryIndexed`.
    void begin_query_indexed(GLuint index) const requires mt::is_mutable
    {
        gl::glBeginQueryIndexed(enum_cast<GLenum>(TargetV), index, self_id());
    }

    // Wraps `glEndQueryIndexed`.
    void end_query_indexed(GLuint index) const requires mt::is_mutable
    {
        gl::glEndQueryIndexed(enum_cast<GLenum>(TargetV), index, self_id());
    }
};


template<typename CRTP, QueryTarget TargetV>
struct PrimaryCapability
    : CapabilityOfBeginEnd<CRTP, TargetV>
    , josh::detail::conditional_mixin_t<
        is_query_indexed<TargetV>::value,
        CapabilityOfBeginEndIndexed<CRTP, TargetV>
    >
{};

template<typename CRTP>
struct PrimaryCapability<CRTP, QueryTarget::Timestamp>
    : CapabilityOfTimestamp<CRTP>
{};

template<typename CRTP, QueryTarget TargetV>
struct Query
    : Common<CRTP, TargetV>
    , PrimaryCapability<CRTP, TargetV>
{};


} // namespace query_api
} // namespace detail



#define JOSH3D_GENERATE_DSA_QUERY_CLASSES(Name)                                                               \
    template<mutability_tag MutT = GLMutable>                                                                 \
    class RawQuery##Name                                                                                      \
        : public detail::RawGLHandle<>                                                                        \
        , public detail::query_api::Query<RawQuery##Name<MutT>, QueryTarget::Name>                            \
    {                                                                                                         \
    public:                                                                                                   \
        static constexpr auto kind_type   = GLKind::Query;                                                    \
        static constexpr auto target_type = QueryTarget::Name;                                                \
        JOSH3D_MAGIC_CONSTRUCTORS_2(RawQuery##Name, mutability_traits<RawQuery##Name>, detail::RawGLHandle<>) \
    };                                                                                                        \
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




} // namespace josh
