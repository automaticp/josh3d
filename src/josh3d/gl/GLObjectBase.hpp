#pragma once
#include <glbinding/gl/gl.h>
#include <concepts>


namespace josh {

/*
Implementation base class for OpenGL resources that
carry a handle: Shaders, Textures, VBOs, etc. RAII-enabled.
*/
template<typename CRTP>
class GLObjectBase {
protected:
	// ID representing a handle to a GL object
    gl::GLuint id_{ 0 };

	// All handles have to be acquired in the constructors of the Derived classes;
	// Example defenition body:
    //     glGenBuffers(1, &id_);
	GLObjectBase() = default;

    // Defines a way to release a handle. Used during move-assignment and in d-tors;
    // Example defenition body:
    //     glDeleteBuffers(1, &id_);
    void release() noexcept = delete;

    friend CRTP;
    // Destructor. Defined here so that you don't have to :^
    ~GLObjectBase() {
        if (id_) {
            static_cast<CRTP&>(*this).release();
        }
    }

public:
	gl::GLuint id() const noexcept { return id_; }

	// Implicit conversion for C API calls
	operator gl::GLuint() const noexcept { return id_; }

	// No accidental conversions to other integral types
	template <std::convertible_to<gl::GLuint> T>
	operator T() const = delete;


	// Disallow copying
	GLObjectBase(const GLObjectBase& other) = delete;
	GLObjectBase& operator=(const GLObjectBase& other) = delete;

    // Allow moving
	GLObjectBase(GLObjectBase&& other) noexcept : id_{other.id_} {
        other.id_ = 0; // Deletion of null-handles must be skipped in the destructor
    }

	GLObjectBase& operator=(GLObjectBase&& other) noexcept {
        if (id_) {
            static_cast<CRTP&>(*this).release();
        }
        id_ = other.id_;
        other.id_ = 0;
        return *this;
    }
};


} // namespace josh
