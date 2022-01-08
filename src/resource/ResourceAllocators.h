#pragma once

#include <glad/glad.h>
#include <iostream>
#include "IResource.h"


class ShaderAllocator : public IResource {
protected:
	explicit ShaderAllocator(GLenum type) noexcept {
		id_ = glCreateShader(type);
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] "
		          << "ShaderAllocator("
		          << ((type == GL_FRAGMENT_SHADER) ? "GL_FRAGMENT_SHADER" : "")
		          << ((type == GL_VERTEX_SHADER) ? "GL_VERTEX_SHADER" : "") << ")";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "ShaderAllocator::release()";
#endif
		glDeleteShader(id_);
	}

public:
	virtual ~ShaderAllocator() override { ShaderAllocator::release(); }

	ShaderAllocator(ShaderAllocator&&) = default;
	ShaderAllocator& operator=(ShaderAllocator&&) = default;
};


class ShaderProgramAllocator : public IResource {
protected:
	ShaderProgramAllocator() noexcept {
		id_ = glCreateProgram();
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] "
		          << "ShaderProgramAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "ShaderProgramAllocator::release()";
#endif
		glDeleteProgram(id_);
	}

public:
	virtual ~ShaderProgramAllocator() override { ShaderProgramAllocator::release(); }

	ShaderProgramAllocator(ShaderProgramAllocator&&) = default;
	ShaderProgramAllocator& operator=(ShaderProgramAllocator&&) = default;
};


class TextureAllocator : public IResource {
protected:
	TextureAllocator() noexcept {
		glGenTextures(1, &id_);
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] "
		          << "TextureAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "TextureAllocator::release()";
#endif
		glDeleteTextures(1, &id_);
	}

public:
	virtual ~TextureAllocator() override { TextureAllocator::release(); }

	TextureAllocator(TextureAllocator&&) = default;
	TextureAllocator& operator=(TextureAllocator&&) = default;
};


class VAOAllocator : public IResource {
protected:
	VAOAllocator() noexcept {
		glGenVertexArrays(1, &id_);
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] "
		          << "VAOAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "VAOAllocator::release()";
#endif
		glDeleteVertexArrays(1, &id_);
	}

public:
	virtual ~VAOAllocator() override { VAOAllocator::release(); }

	VAOAllocator(VAOAllocator&&) = default;
	VAOAllocator& operator=(VAOAllocator&&) = default;
};


class VBOAllocator : public IResource {
protected:
	VBOAllocator() noexcept {
		glGenBuffers(1, &id_);
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] "
		          << "VBOAllocator()";
#endif
	}

private:
	virtual void release() noexcept override final {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "VBOAllocator::release()";
#endif
		glDeleteBuffers(1, &id_);
	}

public:
	virtual ~VBOAllocator() override { VBOAllocator::release(); }

	VBOAllocator(VBOAllocator&&) = default;
	VBOAllocator& operator=(VBOAllocator&&) = default;
};
