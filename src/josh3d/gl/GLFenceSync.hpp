#pragma once
#include "GLAPI.hpp"
#include "GLAPITargets.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "CommonMacros.hpp" // IWYU pragma: keep
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include <chrono>


namespace josh {


enum class SyncWaitResult : GLuint
{
    HasSignaled        = GLuint(gl::GL_CONDITION_SATISFIED),
    HasAlreadySignaled = GLuint(gl::GL_ALREADY_SIGNALED),
    TimeoutExpired     = GLuint(gl::GL_TIMEOUT_EXPIRED),
    WaitFailed         = GLuint(gl::GL_WAIT_FAILED),
};
JOSH3D_DEFINE_ENUM_EXTRAS(SyncWaitResult,
    HasSignaled, HasAlreadySignaled, TimeoutExpired, WaitFailed);


template<mutability_tag MutT = GLMutable>
class RawFenceSync
    : public detail::RawGLHandle<gl::GLsync>
{
public:
    static constexpr auto kind_type   = GLKind::FenceSync;
    static constexpr auto target_type = FenceSyncTarget::GPUCommandsComplete;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawFenceSync, mutability_traits<RawFenceSync>, JOSH3D_SINGLE_ARG(detail::RawGLHandle<gl::GLsync>))

    using nanoseconds = std::chrono::duration<GLuint64, std::nano>;

    // Wraps `glGetSynciv` with `pname = GL_SYNC_STATUS`.
    auto has_signaled() const
        -> bool
    {
        GLint result;
        GLsizei ignore_me;
        gl::glGetSynciv(this->id(), gl::GL_SYNC_STATUS, 1, &ignore_me, &result);
        return result == gl::GL_SIGNALED;
    }

    // Wraps `glClientWaitSync` with no `flags` set.
    [[nodiscard]] auto wait_for(nanoseconds timeout) const
        -> SyncWaitResult
    {
        GLenum result = gl::glClientWaitSync(this->id(), {}, timeout.count());
        return enum_cast<SyncWaitResult>(result);
    }

    // Wraps `glClientWaitSync` with `flags = GL_SYNC_FLUSH_COMMANDS_BIT`.
    [[nodiscard]] auto flush_and_wait_for(nanoseconds timeout) const
        -> SyncWaitResult
    {
        GLenum result = gl::glClientWaitSync(this->id(), gl::GL_SYNC_FLUSH_COMMANDS_BIT, timeout.count());
        return enum_cast<SyncWaitResult>(result);
    }

    // Wraps `glWaitSync`.
    //
    // It is very likely you want to glFlush before this, else
    // the fence might be yet not in the queue.
    void unsafe_stall_cmd_queue_until_signaled() const
    {
        gl::glWaitSync(this->id(), {}, gl::GL_TIMEOUT_IGNORED);
    }

    // Wraps `glFlush` followed by `glWaitSync`.
    void flush_and_stall_cmd_queue_until_signaled() const
    {
        gl::glFlush();
        unsafe_stall_cmd_queue_until_signaled();
    }
};




} // namespace josh
