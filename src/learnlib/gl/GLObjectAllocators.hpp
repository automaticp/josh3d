#pragma once
#include "GLObjectBase.hpp"
#include <glbinding/gl/gl.h>



namespace learn {


/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/

namespace leaksgl {

using namespace gl;

// Allocators - RAII lifetime wrappers for OpenGL objects


class ShaderAllocator : public GLObjectBase<ShaderAllocator> {
public:
	explicit ShaderAllocator(GLenum type) noexcept { id_ = glCreateShader(type); }

private:
	friend class GLObjectBase<ShaderAllocator>;
	void release() noexcept { glDeleteShader(id_); }
};


class ShaderProgramAllocator : public GLObjectBase<ShaderProgramAllocator> {
public:
	ShaderProgramAllocator() noexcept { id_ = glCreateProgram(); }

private:
	friend class GLObjectBase<ShaderProgramAllocator>;
	void release() noexcept { glDeleteProgram(id_); }
};


class TextureAllocator : public GLObjectBase<TextureAllocator> {
public:
	TextureAllocator() noexcept { glGenTextures(1, &id_); }

private:
	friend class GLObjectBase<TextureAllocator>;
    void release() noexcept { glDeleteTextures(1, &id_); }
};


class VAOAllocator : public GLObjectBase<VAOAllocator> {
public:
	VAOAllocator() noexcept { glGenVertexArrays(1, &id_); }

private:
	friend class GLObjectBase<VAOAllocator>;
	void release() noexcept { glDeleteVertexArrays(1, &id_); }
};


class BufferAllocator : public GLObjectBase<BufferAllocator> {
public:
	BufferAllocator() noexcept { glGenBuffers(1, &id_); }

private:
	friend class GLObjectBase<BufferAllocator>;
	void release() noexcept { glDeleteBuffers(1, &id_); }
};


class FramebufferAllocator : public GLObjectBase<FramebufferAllocator> {
public:
	FramebufferAllocator() noexcept { glGenFramebuffers(1, &id_); }

private:
	friend class GLObjectBase<FramebufferAllocator>;
	void release() noexcept { glDeleteFramebuffers(1, &id_); }
};


class RenderbufferAllocator : public GLObjectBase<RenderbufferAllocator> {
public:
	RenderbufferAllocator() noexcept { glGenRenderbuffers(1, &id_); }

private:
	friend class GLObjectBase<RenderbufferAllocator>;
	void release() noexcept { glDeleteRenderbuffers(1, &id_); }
};



} // namespace leaksgl



using leaksgl::ShaderAllocator;
using leaksgl::ShaderProgramAllocator;
using leaksgl::TextureAllocator;
using leaksgl::VAOAllocator;
using leaksgl::BufferAllocator;
using leaksgl::FramebufferAllocator;
using leaksgl::RenderbufferAllocator;


} // namespace learn
