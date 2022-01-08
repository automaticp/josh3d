#pragma once
#include <concepts>
// TODO: implement some actual ownership of the Id through smart pointers with custom deallocators 
//  maybe have like a templated IResource<std::unique_ptr<GLuint, glDeallocator<Derived>>> with CRTP in Derived classes


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
	// FIXME: i have no idea how to implement move assignment here since the operator needs to know
	//  how to release the resource that the object currently holds and that is only known in derived classes.
	//  For now I'm thinking I could make this one default and implement the assignment in the Allocator classes.
	//  There's also an option of creating a virtual method release(), implementing it in the Allocator classes
	//  and calling that.
	//  Calling release from IResource d-tor instead of overriding the destructor in the Allocator classes
	//  will also allow the creation of implicitly defined copy and move c-tors
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

