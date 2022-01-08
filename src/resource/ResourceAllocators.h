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
	virtual void release() noexcept override final { glDeleteShader(id_); }

public:
	virtual ~ShaderAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~ShaderAllocator()" ;
#endif
		ShaderAllocator::release();
	}
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
	virtual void release() noexcept override final { glDeleteProgram(id_); }

public:
	virtual ~ShaderProgramAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~ShaderProgramAllocator()" ;
#endif
		ShaderProgramAllocator::release();
	}
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
	virtual void release() noexcept override final { glDeleteTextures(1, &id_); }

public:
	virtual ~TextureAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~TextureAllocator()" ;
#endif
		TextureAllocator::release();
	}
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
	virtual void release() noexcept override final { glDeleteVertexArrays(1, &id_); }

public:
	virtual ~VAOAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~VAOAllocator()" ;
#endif
		VAOAllocator::release();
	}
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
	virtual void release() noexcept override final { glDeleteBuffers(1, &id_); }

public:
	virtual ~VBOAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~VBOAllocator()" ;
#endif
		VBOAllocator::release();
	}
};
