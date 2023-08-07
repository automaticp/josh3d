#pragma once
#include "GLObjectBase.hpp"
#include "GLScalars.hpp"
#include <glbinding/gl/gl.h>



// Handles - RAII lifetime wrappers for OpenGL objects.
// No additional interface. Implementation base of GLObjects.

namespace josh {


class ShaderHandle : public GLObjectBase<ShaderHandle> {
public:
	explicit ShaderHandle(GLenum type) noexcept { id_ = gl::glCreateShader(type); }

private:
	friend class GLObjectBase<ShaderHandle>;
	void release() noexcept { gl::glDeleteShader(id_); }
};


class ShaderProgramHandle : public GLObjectBase<ShaderProgramHandle> {
public:
	ShaderProgramHandle() noexcept { id_ = gl::glCreateProgram(); }

private:
	friend class GLObjectBase<ShaderProgramHandle>;
	void release() noexcept { gl::glDeleteProgram(id_); }
};


class TextureHandle : public GLObjectBase<TextureHandle> {
public:
	TextureHandle() noexcept { gl::glGenTextures(1, &id_); }

private:
	friend class GLObjectBase<TextureHandle>;
    void release() noexcept { gl::glDeleteTextures(1, &id_); }
};


class VAOHandle : public GLObjectBase<VAOHandle> {
public:
	VAOHandle() noexcept { gl::glGenVertexArrays(1, &id_); }

private:
	friend class GLObjectBase<VAOHandle>;
	void release() noexcept { gl::glDeleteVertexArrays(1, &id_); }
};


class BufferHandle : public GLObjectBase<BufferHandle> {
public:
	BufferHandle() noexcept { gl::glGenBuffers(1, &id_); }

private:
	friend class GLObjectBase<BufferHandle>;
	void release() noexcept { gl::glDeleteBuffers(1, &id_); }
};


class FramebufferHandle : public GLObjectBase<FramebufferHandle> {
public:
	FramebufferHandle() noexcept { gl::glGenFramebuffers(1, &id_); }

private:
	friend class GLObjectBase<FramebufferHandle>;
	void release() noexcept { gl::glDeleteFramebuffers(1, &id_); }
};


class RenderbufferHandle : public GLObjectBase<RenderbufferHandle> {
public:
	RenderbufferHandle() noexcept { gl::glGenRenderbuffers(1, &id_); }

private:
	friend class GLObjectBase<RenderbufferHandle>;
	void release() noexcept { gl::glDeleteRenderbuffers(1, &id_); }
};




} // namespace josh
