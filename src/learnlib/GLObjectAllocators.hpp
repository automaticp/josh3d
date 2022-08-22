#pragma once
#include <concepts>
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


// Base class for OpenGL resources that carry a handle:
// Shaders, Textures, VBOs, etc. RAII-enabled.
template<typename CRTP>
class GLObject {
protected:
	// ID representing a handle to a GL object
    GLuint id_{ 0 };

	// All handles have to be acquired in the constructors of the Derived classes;
	// Example defenition body:
    //     glGenBuffers(1, &id_);
	GLObject() = default;

    // Defines a way to release a handle. Used during move-assignment and in d-tors;
    // Example defenition body:
    //     glDeleteBuffers(1, &id_);
    void release() noexcept = delete;

    friend class CRTP;
    // Destructor. Defined here so that you don't have to :^
    ~GLObject() {
        if (id_) {
            static_cast<CRTP&>(*this).release();
        }
    }

public:
	GLuint id() const noexcept { return id_; }

	// Implicit conversion for C API calls
	operator GLuint() const noexcept { return id_; }

	// No accidental conversions to other integral types
	template <typename T> requires std::convertible_to<T, GLuint>
	operator T() const = delete;


	// Disallow copying
	GLObject(const GLObject& other) = delete;
	GLObject& operator=(const GLObject& other) = delete;

    // Allow moving
	GLObject(GLObject&& other) noexcept : id_{other.id_} {
        other.id_ = 0; // Deletion of null-handles must be skipped in the destructor
    }

	GLObject& operator=(GLObject&& other) noexcept {
        static_cast<CRTP&>(*this).release();
        id_ = other.id;
        other.id_ = 0;
        return *this;
    }
};




// Allocators - RAII lifetime wrappers for OpenGL objects


class ShaderAllocator : public GLObject<ShaderAllocator> {
public:
	explicit ShaderAllocator(GLenum type) noexcept { id_ = glCreateShader(type); }

private:
	void release() noexcept { glDeleteShader(id_); }
};


class ShaderProgramAllocator : public GLObject<ShaderProgramAllocator> {
public:
	ShaderProgramAllocator() noexcept { id_ = glCreateProgram(); }

private:
	void release() noexcept { glDeleteProgram(id_); }
};


class TextureAllocator : public GLObject<TextureAllocator> {
public:
	TextureAllocator() noexcept { glGenTextures(1, &id_); }

private:
    void release() noexcept { glDeleteTextures(1, &id_); }
};


class VAOAllocator : public GLObject<VAOAllocator> {
public:
	VAOAllocator() noexcept { glGenVertexArrays(1, &id_); }

private:
	void release() noexcept { glDeleteVertexArrays(1, &id_); }
};


class BufferAllocator : public GLObject<BufferAllocator> {
public:
	BufferAllocator() noexcept { glGenBuffers(1, &id_); }

private:
	void release() noexcept { glDeleteBuffers(1, &id_); }
};


} // namespace leaksgl


template<typename CRTP>
using GLObject = leaksgl::GLObject<CRTP>; // what?

using leaksgl::ShaderAllocator;
using leaksgl::ShaderProgramAllocator;
using leaksgl::TextureAllocator;
using leaksgl::VAOAllocator;
using leaksgl::BufferAllocator;


} // namespace learn
