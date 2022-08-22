#pragma once
#include <concepts>
#include <glbinding/gl/gl.h>
// TODO: implement some actual ownership of the Id through smart pointers with custom deallocators
//  maybe have like a templated IResource<std::unique_ptr<GLuint, glDeallocator<Derived>>> with CRTP in Derived classes

using namespace gl;

// Base class for OpenGL resources that carry a handle (Shaders, Textures, VBOs, etc.). RAII-enabled.
class IResource {
protected:
	GLuint id_{ 0 }; // null-defaulted as deleting a null handle has no effect in OpenGL

public:
	GLuint getId() const noexcept { return id_; }

	// implicit conversion for C API calls
	operator GLuint() const noexcept { return id_; }

	// no accidental conversions to other integral types
	template <typename T> requires std::convertible_to<T, GLuint>
	operator T() const = delete;

	// all resources have to be acquired in the constructors of the Derived classes;
	//     Example defenition body: glGenBuffers(1, &id_);
	IResource() = default;

	// disallow copying
	IResource(const IResource& other) = delete;
	IResource& operator=(const IResource& other) = delete;
	// allow moving
	IResource(IResource&& other) noexcept;
	IResource& operator=(IResource&& other) noexcept;

protected:
	// wraps the call to OpenGL to release a resource;
	// defined separate from the d-tor for use in move-assignment
	//     Example defenition body: glDeleteBuffers(1, &id_);
	virtual void release() noexcept = 0;

public:
	// all resources have to be released in the destructors of the derived classes;
	//      Derived d-tor body: Derived::release();
	// this calls local non-virtual copy of the release() method
	virtual ~IResource() = 0;

};

inline IResource::IResource(IResource&& other) noexcept : id_{other.id_} {
	other.id_ = 0; // deletion of null-handles is silently ignored in OGL
}

inline IResource& IResource::operator=(IResource&& other) noexcept {
	this->release();
	id_ = other.id_;
	other.id_ = 0;
	return *this;
}

inline IResource::~IResource() = default;

