#pragma once
#include "GLMutability.hpp" // IWYU pragma: keep (concepts)
#include "GLKindHandles.hpp"
#include "GLScalars.hpp"
#include <chrono>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions-patches.h>
#include <glbinding/gl/functions.h>


namespace josh {


template<mutability_tag MutT = GLMutable>
class RawTimerQuery
    : public RawQueryHandle<MutT>
    // FIXME: This is useless everywhere. Given that we have mutability traits.
    , public detail::ObjectHandleTypeInfo<RawTimerQuery, MutT>
{
public:
    JOSH3D_MAGIC_CONSTRUCTORS(RawTimerQuery, MutT, RawQueryHandle<MutT>)

    void begin_query() const noexcept
        requires gl_mutable<MutT>
    {
        gl::glBeginQuery(gl::GL_TIME_ELAPSED, this->id());
    }

    void end_query() const noexcept
        requires gl_mutable<MutT>
    {
        gl::glEndQuery(gl::GL_TIME_ELAPSED);
    }

    bool is_available() const noexcept {
        GLboolean result;
        gl::glGetQueryObjectiv(this->id(), gl::GL_QUERY_RESULT_AVAILABLE, &result);
        return result != gl::GL_FALSE;
    }

    // Will stall if the result is not yet available.
    auto result() const noexcept
        -> std::chrono::nanoseconds
    {
        GLint64 result;
        gl::glGetQueryObjecti64v(this->id(), gl::GL_QUERY_RESULT, &result);
        return std::chrono::nanoseconds(result);
    }

};


} // namespace josh
