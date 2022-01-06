#pragma once
#include <concepts>
// TODO: implement some actual ownership of the Id through smart pointers with custom deallocators 
// maybe have like a templated glResource<std::unique_ptr<GLuint, glDeallocator<Derived>>> with CRTP in Derived classes

// Base class for OpenGL resources that carry a handle (Shaders, Textures, VBOs, etc.). RAII-enabled.
class glResource {
protected:
	GLuint m_id{ 0 }; // null-defaulted as deleting a null handle has no effect in OpenGL

public:
	GLuint getId() const noexcept { return m_id; }

	// implicit conversion for C API calls
	operator GLuint() const noexcept { return m_id; }

	// no accidental conversions to other integral types
	template <typename T> requires std::convertible_to<T, GLuint>
	operator T() const = delete;

	// all resources have to be acquired in the constructors of the Derived classes;
	//     Example defenition body: glGenBuffers(1, &m_id);
	glResource() = default;

	// disallow copying, allow moving
	glResource(const glResource& other) = delete;
	glResource& operator=(const glResource& other) = delete;
	glResource(glResource&& other) = default;
	glResource& operator=(glResource&& other) = default;

	// wraps the call to OpenGL to release a resource;
	// all resources have to be released in the virtual destructors of the Derived classes;
	//     Example defenition body: glDeleteBuffers(1, &m_id);
	virtual ~glResource() = 0;

};

