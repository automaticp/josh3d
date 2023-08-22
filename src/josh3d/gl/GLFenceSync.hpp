#pragma once
#include "GLScalars.hpp"
#include "EnumUtils.hpp"
#include <glbinding/gl/gl.h>
#include <chrono>
#include <ratio>
#include <type_traits>
#include <utility>


namespace josh {



class FenceSyncHandle {
protected:
    gl::GLsync id_;

    struct EmptyTag {};
    FenceSyncHandle(EmptyTag) : id_{ nullptr } {}
    FenceSyncHandle() : id_{ create_impl() } {}

    static gl::GLsync create_impl() noexcept {
        return gl::glFenceSync(gl::GL_SYNC_GPU_COMMANDS_COMPLETE, {});
    }

    void release_impl() noexcept {
        gl::glDeleteSync(id_);
    }

public:
    gl::GLsync id() const noexcept { return id_; }
    [[nodiscard]] static FenceSyncHandle create() noexcept { return {}; }
    [[nodiscard]] static FenceSyncHandle create_empty() noexcept { return { EmptyTag{} }; }

    void reset() noexcept {
        release_impl();
        id_ = create_impl();
    }

    void release() noexcept {
        if (id_) release_impl();
        id_ = nullptr;
    }

    FenceSyncHandle(const FenceSyncHandle&)            = delete;
    FenceSyncHandle& operator=(const FenceSyncHandle&) = delete;

    FenceSyncHandle(FenceSyncHandle&& other) noexcept
        : id_{ std::exchange(other.id_, nullptr) }
    {}

    FenceSyncHandle& operator=(FenceSyncHandle&& other) noexcept {
        if (id_) release_impl();
        id_ = std::exchange(other.id_, nullptr);
        return *this;
    }

    ~FenceSyncHandle() noexcept { if (id_) release_impl(); }

};



enum class SyncWaitResult : std::underlying_type_t<GLenum> {
    signaled         = to_underlying(gl::GL_CONDITION_SATISFIED),
    already_signaled = to_underlying(gl::GL_ALREADY_SIGNALED),
    timeout          = to_underlying(gl::GL_TIMEOUT_EXPIRED),
    fail             = to_underlying(gl::GL_WAIT_FAILED),
};


class FenceSync : public FenceSyncHandle {
private:
    using FenceSyncHandle::FenceSyncHandle;

public:
    using nanoseconds = std::chrono::duration<GLuint64, std::nano>;

    [[nodiscard]] static FenceSync create() noexcept { return {}; }
    [[nodiscard]] static FenceSync create_empty() noexcept { return { EmptyTag{} }; }

    bool has_signaled() const noexcept {
        GLint result[1];
        GLsizei ignore_me;
        gl::glGetSynciv(id_, gl::GL_SYNC_STATUS, 1, &ignore_me, result);
        return result[0] == gl::GL_SIGNALED;
    }

    [[nodiscard]]
    SyncWaitResult wait_for(nanoseconds timeout) const noexcept {
        return SyncWaitResult{
            to_underlying(gl::glClientWaitSync(id_, {}, timeout.count()))
        };
    }

    [[nodiscard]]
    SyncWaitResult flush_and_wait_for(nanoseconds timeout) const noexcept {
        return SyncWaitResult{
            to_underlying(gl::glClientWaitSync(id_, gl::GL_SYNC_FLUSH_COMMANDS_BIT, timeout.count()))
        };
    }

    // It is very likely you want to glFlush before this, else
    // the fence might be yet not in the queue.
    void stall_cmd_queue_until_signaled() const noexcept {
        gl::glWaitSync(id_, {}, gl::GL_TIMEOUT_IGNORED);
    }

};




} // namespace josh
