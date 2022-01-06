#pragma once
#include <concepts>
#include "TypeAliases.h"
// TODO: implement some actual ownership of the Id through smart pointers with custom deallocators 
// maybe have like a templated glResource<std::unique_ptr<GLuint, glDeallocator<Derived>>> with CRTP in Derived classes
// TODO: maybe also allow for dynamic memory managment though "std::shared_ptr<bool> m_isDestroyed" and conditional destructor;
// the above idea sound like some dangerous garbo, tbh

class glResource {
protected:
	GLuint m_id{ 0 };

	// wraps the call to OpenGL to acquire a resource;
	// all resources have to be acquired in the constructors of the Derived classes;
	//     Example defenition: glGenBuffers(1, &m_id);
	// I also think that this being a virtual method is somewhat unneccesary
	// except for forcing the user to define their own acquireResource();
	virtual void acquireResource() = 0;

	// wraps the call to OpenGL to release a resource; 
	//     Example defenition: glDeleteBuffers(1, &m_id);
	virtual void releaseResource() = 0;

public:
	// all resources can be destroyed at leisure 
	// all resources must be destroyed manually (no u-d destructor; hopefully, will change later)
	void destroy() {
		releaseResource();
	};

	GLuint getId() const noexcept { return m_id; }

	// implicit conversion for C API calls
	operator GLuint() const noexcept { return m_id; }

	// no accidental conversions to other integral types
	template <typename T> requires std::convertible_to<T, GLuint>
	operator T() const = delete; 

	glResource() = default;
	// disallow copying, allow moving
	glResource(const glResource& other) = delete;
	glResource& operator=(const glResource& other) = delete;
	glResource(glResource&& other) = default;
	glResource& operator=(glResource&& other) = default;
	virtual ~glResource() = default;

};

