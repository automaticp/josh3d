#pragma once
#include "GLAPI.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "SingleArgMacro.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <chrono>


namespace josh {



enum class SyncWaitResult : GLuint {
    HasSignaled        = GLuint(gl::GL_CONDITION_SATISFIED),
    HasAlreadySignaled = GLuint(gl::GL_ALREADY_SIGNALED),
    TimeoutExpired     = GLuint(gl::GL_TIMEOUT_EXPIRED),
    WaitFailed         = GLuint(gl::GL_WAIT_FAILED),
};




template<mutability_tag MutT = GLMutable>
class RawFenceSync
    : public detail::RawGLHandle<MutT, gl::GLsync>
{
public:
    static constexpr GLKind kind_type   = GLKind::FenceSync;
    static constexpr GLenum target_type = gl::GL_SYNC_GPU_COMMANDS_COMPLETE;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawFenceSync, mutability_traits<RawFenceSync>, JOSH3D_SINGLE_ARG(detail::RawGLHandle<MutT, gl::GLsync>))

    using nanoseconds = std::chrono::duration<GLuint64, std::nano>;

    // Wraps `glGetSynciv` with `pname = GL_SYNC_STATUS`.
    bool has_signaled() const noexcept {
        GLint result;
        GLsizei ignore_me;
        gl::glGetSynciv(this->id(), gl::GL_SYNC_STATUS, 1, &ignore_me, &result);
        return result == gl::GL_SIGNALED;
    }

    // Wraps `glClientWaitSync` with no `flags` set.
    [[nodiscard]] SyncWaitResult wait_for(nanoseconds timeout) const noexcept {
        GLenum result = gl::glClientWaitSync(this->id(), {}, timeout.count());
        return enum_cast<SyncWaitResult>(result);
    }

    // Wraps `glClientWaitSync` with `flags = GL_SYNC_FLUSH_COMMANDS_BIT`.
    [[nodiscard]] SyncWaitResult flush_and_wait_for(nanoseconds timeout) const noexcept {
        GLenum result = gl::glClientWaitSync(this->id(), gl::GL_SYNC_FLUSH_COMMANDS_BIT, timeout.count());
        return enum_cast<SyncWaitResult>(result);
    }

    // Wraps `glWaitSync`.
    //
    // It is very likely you want to glFlush before this, else
    // the fence might be yet not in the queue.
    void unsafe_stall_cmd_queue_until_signaled() const noexcept {
        gl::glWaitSync(this->id(), {}, gl::GL_TIMEOUT_IGNORED);
    }

    // Wraps `glFlush` followed by `glWaitSync`.
    void flush_and_stall_cmd_queue_until_signaled() const noexcept {
        gl::glFlush();
        unsafe_stall_cmd_queue_until_signaled();
    }

};




} // namespace josh
