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
public:
	virtual ~ShaderAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~ShaderAllocator()" ;
#endif
		glDeleteShader(id_);
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

public:
	virtual ~ShaderProgramAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~ShaderProgramAllocator()" ;
#endif
		glDeleteProgram(id_);
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

public:
	virtual ~TextureAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~TextureAllocator()" ;
#endif
		glDeleteTextures(1, &id_);
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

public:
	virtual ~VAOAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~VAOAllocator()" ;
#endif
		glDeleteVertexArrays(1, &id_);
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

public:
	virtual ~VBOAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~VBOAllocator()" ;
#endif
		glDeleteBuffers(1, &id_);
	}
};
