#pragma once
#include "GLAPI.hpp"
#include "GLScalars.hpp"
#include "GLKind.hpp"
#include <glbinding/gl/functions.h>


namespace josh::dsa {


namespace detail {


struct TextureAllocator {
    using request_arg_type = GLenum;
    static GLuint request(GLenum target) noexcept {
        GLuint id;
        gl::glCreateTextures(target, 1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteTextures(1, &id);
    }
};

struct BufferAllocator {
    using request_arg_type = void;
    static GLuint request() noexcept {
        GLuint id;
        gl::glCreateBuffers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteBuffers(1, &id);
    }
};

struct VertexArrayAllocator {
    using request_arg_type = void;
    static GLuint request() noexcept {
        GLuint id;
        gl::glCreateVertexArrays(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteVertexArrays(1, &id);
    }
};

struct FramebufferAllocator {
    using request_arg_type = void;
    static GLuint request() noexcept {
        GLuint id;
        gl::glCreateFramebuffers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteFramebuffers(1, &id);
    }
};

struct RenderbufferAllocator {
    using request_arg_type = void;
    static GLuint request() noexcept {
        GLuint id;
        gl::glCreateRenderbuffers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteRenderbuffers(1, &id);
    }
};

struct ShaderAllocator {
    using request_arg_type = GLenum;
    static GLuint request(GLenum shader_type) noexcept {
        return gl::glCreateShader(shader_type);
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteShader(id);
    }
};

struct ProgramAllocator {
    using request_arg_type = void;
    static GLuint request() noexcept {
        return gl::glCreateProgram();
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteProgram(id);
    }
};

struct FenceSyncAllocator {
    using request_arg_type = GLenum;
    static gl::GLsync request(GLenum target) noexcept {
        return gl::glFenceSync(target, {});
    }
    static void release(gl::GLsync id) noexcept {
        gl::glDeleteSync(id);
    }
};

struct QueryAllocator {
    using request_arg_type = GLenum;
    static GLuint request(GLenum target) noexcept {
        GLuint id;
        gl::glCreateQueries(target, 1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteQueries(1, &id);
    }
};

struct SamplerAllocator {
    using request_arg_type = void;
    static GLuint request() noexcept {
        GLuint id;
        gl::glCreateSamplers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteSamplers(1, &id);
    }
};


} // namespace detail



/*
Allocator type that defines primary allocation facilities
for each particulat Kind of object.
*/
template<GLKind KindV>
struct GLAllocator;

#define JOSH3D_SPECIALIZE_DSA_ALLOCATOR(kind) \
    template<>                                \
    struct GLAllocator<GLKind::kind>          \
        : detail::kind##Allocator             \
    {};


JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Texture)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Buffer)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(VertexArray)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Framebuffer)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Renderbuffer)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Shader)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Program)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(FenceSync)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Query)
JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Sampler)

#undef JOSH3D_SPECIALIZE_DSA_ALLOCATOR




} // namespace josh::dsa
