#pragma once
#include "CommonConcepts.hpp"
#include "GLAPI.hpp"
#include "GLAPITargets.hpp"
#include "GLScalars.hpp"
#include "GLKind.hpp"
#include <concepts>


namespace josh {
namespace detail {


struct TextureAllocator
{
    using request_arg_type = TextureTarget;
    static auto request(request_arg_type target) -> GLuint { GLuint id; gl::glCreateTextures(enum_cast<GLenum>(target), 1, &id); return id; }
    static void release(GLuint id) { gl::glDeleteTextures(1, &id); }
};

struct BufferAllocator
{
    using request_arg_type = void;
    static auto request(request_arg_type) -> GLuint { GLuint id; gl::glCreateBuffers(1, &id); return id; }
    static void release(GLuint id) { gl::glDeleteBuffers(1, &id); }
};

struct VertexArrayAllocator
{
    using request_arg_type = void;
    static auto request(request_arg_type) -> GLuint { GLuint id; gl::glCreateVertexArrays(1, &id); return id; }
    static void release(GLuint id) { gl::glDeleteVertexArrays(1, &id); }
};

struct FramebufferAllocator
{
    using request_arg_type = void;
    static auto request(request_arg_type) -> GLuint { GLuint id; gl::glCreateFramebuffers(1, &id); return id; }
    static void release(GLuint id) { gl::glDeleteFramebuffers(1, &id); }
};

struct RenderbufferAllocator
{
    using request_arg_type = void;
    static auto request(request_arg_type) -> GLuint { GLuint id; gl::glCreateRenderbuffers(1, &id); return id; }
    static void release(GLuint id) { gl::glDeleteRenderbuffers(1, &id); }
};

struct ShaderAllocator
{
    using request_arg_type = ShaderTarget;
    static auto request(request_arg_type target) -> GLuint { return gl::glCreateShader(enum_cast<GLenum>(target)); }
    static void release(GLuint id) { gl::glDeleteShader(id); }
};

struct ProgramAllocator
{
    using request_arg_type = void;
    static auto request(request_arg_type) -> GLuint { return gl::glCreateProgram(); }
    static void release(GLuint id) { gl::glDeleteProgram(id); }
};

struct FenceSyncAllocator
{
    using request_arg_type = FenceSyncTarget;
    static auto request(request_arg_type target) -> gl::GLsync { return gl::glFenceSync(enum_cast<GLenum>(target), {}); }
    static void release(gl::GLsync id) { gl::glDeleteSync(id); }
};

struct QueryAllocator
{
    using request_arg_type = QueryTarget;
    static auto request(request_arg_type target) -> GLuint { GLuint id; gl::glCreateQueries(enum_cast<GLenum>(target), 1, &id); return id; }
    static void release(GLuint id) { gl::glDeleteQueries(1, &id); }
};

struct SamplerAllocator
{
    using request_arg_type = void;
    static auto request(request_arg_type) -> GLuint { GLuint id; gl::glCreateSamplers(1, &id); return id; }
    static void release(GLuint id) { gl::glDeleteSamplers(1, &id); }
};

} // namespace detail


/*
Allocator type that defines primary allocation facilities
for each particulat Kind of object.
*/
template<GLKind KindV>
struct GLAllocator;

#define JOSH3D_SPECIALIZE_DSA_ALLOCATOR(Kind) \
    template<>                                \
    struct GLAllocator<GLKind::Kind>          \
        : detail::Kind##Allocator             \
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

template<typename RawH>
concept supports_gl_allocator = requires
{
    { RawH::kind_type } -> std::convertible_to<GLKind>;
    typename GLAllocator<RawH::kind_type>;
};


} // namespace josh
