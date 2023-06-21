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

// Handles - RAII lifetime wrappers for OpenGL objects.
// No additional interface. Implementation base of GLObjects.


class ShaderHandle : public GLObjectBase<ShaderHandle> {
public:
	explicit ShaderHandle(GLenum type) noexcept { id_ = glCreateShader(type); }

private:
	friend class GLObjectBase<ShaderHandle>;
	void release() noexcept { glDeleteShader(id_); }
};


class ShaderProgramHandle : public GLObjectBase<ShaderProgramHandle> {
public:
	ShaderProgramHandle() noexcept { id_ = glCreateProgram(); }

private:
	friend class GLObjectBase<ShaderProgramHandle>;
	void release() noexcept { glDeleteProgram(id_); }
};


class TextureHandle : public GLObjectBase<TextureHandle> {
public:
	TextureHandle() noexcept { glGenTextures(1, &id_); }

private:
	friend class GLObjectBase<TextureHandle>;
    void release() noexcept { glDeleteTextures(1, &id_); }
};


class VAOHandle : public GLObjectBase<VAOHandle> {
public:
	VAOHandle() noexcept { glGenVertexArrays(1, &id_); }

private:
	friend class GLObjectBase<VAOHandle>;
	void release() noexcept { glDeleteVertexArrays(1, &id_); }
};


class BufferHandle : public GLObjectBase<BufferHandle> {
public:
	BufferHandle() noexcept { glGenBuffers(1, &id_); }

private:
	friend class GLObjectBase<BufferHandle>;
	void release() noexcept { glDeleteBuffers(1, &id_); }
};


class FramebufferHandle : public GLObjectBase<FramebufferHandle> {
public:
	FramebufferHandle() noexcept { glGenFramebuffers(1, &id_); }

private:
	friend class GLObjectBase<FramebufferHandle>;
	void release() noexcept { glDeleteFramebuffers(1, &id_); }
};


class RenderbufferHandle : public GLObjectBase<RenderbufferHandle> {
public:
	RenderbufferHandle() noexcept { glGenRenderbuffers(1, &id_); }

private:
	friend class GLObjectBase<RenderbufferHandle>;
	void release() noexcept { glDeleteRenderbuffers(1, &id_); }
};



} // namespace leaksgl



using leaksgl::ShaderHandle;
using leaksgl::ShaderProgramHandle;
using leaksgl::TextureHandle;
using leaksgl::VAOHandle;
using leaksgl::BufferHandle;
using leaksgl::FramebufferHandle;
using leaksgl::RenderbufferHandle;


} // namespace learn
