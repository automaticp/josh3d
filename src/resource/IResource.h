#pragma once
#include <concepts>
// TODO: implement some actual ownership of the Id through smart pointers with custom deallocators 
// maybe have like a templated IResource<std::unique_ptr<GLuint, glDeallocator<Derived>>> with CRTP in Derived classes

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

	// disallow copying, allow moving
	IResource(const IResource& other) = delete;
	IResource& operator=(const IResource& other) = delete;
	IResource(IResource&& other) = default;
	IResource& operator=(IResource&& other) = default;

	// wraps the call to OpenGL to release a resource;
	// all resources have to be released in the virtual destructors of the Derived classes;
	//     Example defenition body: glDeleteBuffers(1, &id_);
	virtual ~IResource() = 0;

};

inline IResource::~IResource() = default;

