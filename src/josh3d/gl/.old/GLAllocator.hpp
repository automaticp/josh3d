#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLMutability.hpp"   // IWYU pragma: keep
#include "GLKindHandles.hpp"
#include "GLScalars.hpp"
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <concepts>
#include <glbinding/gl/types.h>


namespace josh {


namespace detail {


struct TextureAllocator {
    static GLuint request() noexcept {
        GLuint id;
        gl::glGenTextures(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteTextures(1, &id);
    }
};

struct BufferAllocator {
    static GLuint request() noexcept {
        GLuint id;
        gl::glGenBuffers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteBuffers(1, &id);
    }
};

struct VertexArrayAllocator {
    static GLuint request() noexcept {
        GLuint id;
        gl::glGenVertexArrays(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteVertexArrays(1, &id);
    }
};

struct FramebufferAllocator {
    static GLuint request() noexcept {
        GLuint id;
        gl::glGenFramebuffers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteFramebuffers(1, &id);
    }
};

struct RenderbufferAllocator {
    static GLuint request() noexcept {
        GLuint id;
        gl::glGenRenderbuffers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteRenderbuffers(1, &id);
    }
};

struct ShaderAllocator {
    static GLuint request(GLenum shader_type) noexcept {
        return gl::glCreateShader(shader_type);
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteShader(id);
    }
};

struct ShaderProgramAllocator {
    static GLuint request() noexcept {
        return gl::glCreateProgram();
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteProgram(id);
    }
};

struct QueryAllocator {
    static GLuint request() noexcept {
        GLuint id;
        gl::glGenQueries(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteQueries(1, &id);
    }
};

struct SamplerAllocator {
    static GLuint request() noexcept {
        GLuint id;
        gl::glGenSamplers(1, &id);
        return id;
    }
    static void release(GLuint id) noexcept {
        gl::glDeleteSamplers(1, &id);
    }
};

} // namespace detail




/*
OpenGL object allocator defined for different "kinds" of objects in the API
based on their RawKindHandle type.
*/
template<raw_gl_kind_handle RawKindH>
struct GLAllocator;

#define JOSH3D_SPECIALIZE_ALLOCATOR(kind)            \
    template<mutability_tag MutT>                    \
    struct GLAllocator<Raw##kind##Handle<MutT>>      \
        : detail::kind##Allocator                    \
    {};


JOSH3D_SPECIALIZE_ALLOCATOR(Texture)
JOSH3D_SPECIALIZE_ALLOCATOR(Buffer)
JOSH3D_SPECIALIZE_ALLOCATOR(VertexArray)
JOSH3D_SPECIALIZE_ALLOCATOR(Framebuffer)
JOSH3D_SPECIALIZE_ALLOCATOR(Renderbuffer)
JOSH3D_SPECIALIZE_ALLOCATOR(Shader)
JOSH3D_SPECIALIZE_ALLOCATOR(ShaderProgram)
JOSH3D_SPECIALIZE_ALLOCATOR(Query)
JOSH3D_SPECIALIZE_ALLOCATOR(Sampler)

#undef JOSH3D_SPECIALIZE_ALLOCATOR



template<typename RawKindH>
concept has_gl_allocator = requires {
    sizeof(GLAllocator<RawKindH>);
};


template<typename RawKindH>
concept allocatable_gl_kind_handle =
    raw_gl_kind_handle<RawKindH> &&
    has_gl_allocator<RawKindH>;


template<typename RawObjH>
concept allocatable_gl_object_handle =
    raw_gl_object_handle<RawObjH> &&
    has_gl_allocator<typename RawObjH::kind_handle_type>;



} // namespace josh
