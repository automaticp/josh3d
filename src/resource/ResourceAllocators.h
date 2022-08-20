#pragma once
#include <glbinding/gl/gl.h>
#include <iostream>
#include "IResource.h"

using namespace gl;

class ShaderAllocator : public IResource {
public:
	explicit ShaderAllocator(GLenum type) noexcept {
		id_ = glCreateShader(type);
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] "
		          << "ShaderAllocator("
		          << ((type == GL_FRAGMENT_SHADER) ? "GL_FRAGMENT_SHADER" : "")
		          << ((type == GL_VERTEX_SHADER) ? "GL_VERTEX_SHADER" : "") << ")";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] " << "ShaderAllocator::release()";
#endif
		glDeleteShader(id_);
	}

public:
	virtual ~ShaderAllocator() override { if (id_) { ShaderAllocator::release(); } }

	ShaderAllocator(ShaderAllocator&&) = default;
	ShaderAllocator& operator=(ShaderAllocator&&) = default;
};


class ShaderProgramAllocator : public IResource {
public:
	ShaderProgramAllocator() noexcept {
		id_ = glCreateProgram();
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] "
		          << "ShaderProgramAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] " << "ShaderProgramAllocator::release()";
#endif
		glDeleteProgram(id_);
	}

public:
	virtual ~ShaderProgramAllocator() override { if (id_) { ShaderProgramAllocator::release(); } }

	ShaderProgramAllocator(ShaderProgramAllocator&&) = default;
	ShaderProgramAllocator& operator=(ShaderProgramAllocator&&) = default;
};


class TextureAllocator : public IResource {
public:
	TextureAllocator() noexcept {
		glGenTextures(1, &id_);
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] "
		          << "TextureAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] " << "TextureAllocator::release()";
#endif
		glDeleteTextures(1, &id_);
	}

public:
	virtual ~TextureAllocator() override { if (id_) {  TextureAllocator::release(); } }

	TextureAllocator(TextureAllocator&&) = default;
	TextureAllocator& operator=(TextureAllocator&&) = default;
};


class VAOAllocator : public IResource {
public:
	VAOAllocator() noexcept {
		glGenVertexArrays(1, &id_);
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] "
		          << "VAOAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] " << "VAOAllocator::release()";
#endif
		glDeleteVertexArrays(1, &id_);
	}

public:
	virtual ~VAOAllocator() override { if (id_) {  VAOAllocator::release(); } }

	VAOAllocator(VAOAllocator&&) = default;
	VAOAllocator& operator=(VAOAllocator&&) = default;
};


class VBOAllocator : public IResource {
public:
	VBOAllocator() noexcept {
		glGenBuffers(1, &id_);
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] "
		          << "VBOAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifdef LEARN_ENABLE_LOGGING
		std::cerr << "\n[id: " << id_ << "] " << "VBOAllocator::release()";
#endif
		glDeleteBuffers(1, &id_);
	}

public:
	virtual ~VBOAllocator() override { if (id_) { VBOAllocator::release(); } }

	VBOAllocator(VBOAllocator&&) = default;
	VBOAllocator& operator=(VBOAllocator&&) = default;
};
