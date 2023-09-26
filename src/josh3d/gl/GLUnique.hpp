#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAllocator.hpp"
#include <concepts>


namespace josh {


/*
RawGLHandle
|
RawKindHandle (RawTextureHandle)
|
RawTargetHandle (RawTexture2D)
|
GLUnique? GLShared? GLValue? Any semantics you like.
*/


namespace detail {

template<typename AllocT, typename ...AllocArgs>
concept gl_allocator_request_args = requires(AllocT alloc, AllocArgs&&... args) {
    { alloc.request(std::forward<AllocArgs>(args)...) } -> std::same_as<GLuint>;
};

} // namespace detail




/*
There are 3 ways to expose the underlying Raw objects through a semantic
container like GLUnique or GLValue:
    1. by public inheritance or
    2. by defining operator->/operator* or
    3. by defining an access member function like get()


1. The inheritance is nice because it lets us keep the simple syntax
of member access thorgh `operator.`, but we cannot define out own
`operator.`, and by extension, cannot enable const-propagation
that way.


2. The `operator->` allows us to propagate const. However, if the RawHandle
types decide to NOT propagate const but will use the same `operator->`,
there will be a inconsistency (maybe?) in the interfaces.

Not propagating const for RawHandles means that there has to be separate
mechanism for establishing mutability like GLConst/GLMutable, which
is another layer of crap.


Realisation: Also `operator->` sucks for this since it recursively
calls `->` on the return value until it encounters a pointer type.
Then it dereferences that pointer, which is equivalent to access:
    (*p).member
Where pointer `p` HAS to be a C++ pointer type, meaning it refers to
an actual memory location. This won't work on types that just overload
`operator*`, and OpenGL handles are NOT pointers and have no memory location
associated with them. `operator->` is pretty much hardcoded into the C++
object model and cannot be used with some fancy pointers,
which is truly a "language bad" moment for me.


3. This is better than the other options, but it is verbose and adds a layer
of indirection in the interface, which is especially odd for value containers.
Like, yeah, they might behave like values for regularity under copy,
but the actual functionality will have to be accessed through some get()
function. They're maybe value-like, but are not like C++ values.

    GLValue<RawTexture2D<GLMutable>> texture;
    texture.get().bind().specify_image(...)
    //      ^ who are you?

At least it allows you to propagate const by returning a type with a specific GLMutability:

    RawTexture2D<GLMutable> get()       noexcept { return *this; }
    RawTexture2D<GLConst>   get() const noexcept { return *this; }



To be clear, I don't particularly like either option. The C++ does not have great
support for building types with semantics similar to built-in references and pointers,
which are actually independent from the object model of the language.
Other languages are even worse at this, so don't worry.
*/




/*
Semantic container for OpenGL handles with unique ownership.
Manages the lifetime of stored objects similar to `unique_ptr`.

Does not allow manual resets of the underlying handle.
Allocates the objects at construction.

Does not propagate language-level constness.
Supports GLMutable->GLConst rvalue conversions.

Allows slicing down to the underlying RawObject type.


TODO: Support for kind-handles.
Currently doesn't work because kind handles have no object_handle_type_template.
*/
template<allocatable_gl_object_handle RawObjT>
class GLUnique
    : public  RawObjT
    , private GLAllocator<typename RawObjT::kind_handle_type>
{
private:
    using object_type     = RawObjT;
    using allocator_type  = GLAllocator<typename RawObjT::kind_handle_type>;
    using mutability_type = RawObjT::mutability_type;
    using const_type      = RawObjT::template object_handle_type_template<GLConst>;
    using mutable_type    = RawObjT::template object_handle_type_template<GLMutable>;
    using opposite_type   = RawObjT::template object_handle_type_template<OppositeGLMutability<mutability_type>>;
    friend GLUnique<opposite_type>;

public:
    template<typename ...AllocArgs>
        requires not_move_or_copy_constructor_of<GLUnique, AllocArgs...> &&
            detail::gl_allocator_request_args<allocator_type, AllocArgs...>
    GLUnique(AllocArgs&&... args)
        : RawObjT{ this->allocator_type::request(std::forward<AllocArgs>(args)...) }
    {}

    GLUnique(const GLUnique&)            = delete;
    GLUnique& operator=(const GLUnique&) = delete;

    GLUnique(GLUnique&& other) noexcept
        : object_type{ other.object_type::reset_id() }
    {}

    GLUnique& operator=(GLUnique&& other) noexcept {
        release_current();
        this->object_type::reset_id(other.object_type::reset_id());
        return *this;
    }

    // GLMutable -> GLConst converting move c-tor.
    GLUnique(GLUnique<mutable_type>&& other) noexcept
        requires std::same_as<object_type, const_type>
        : object_type{ other.object_type::reset_id() }
    {}

    // GLMutable -> GLConst converting move assignment.
    GLUnique& operator=(GLUnique<mutable_type>&& other) noexcept
        requires std::same_as<object_type, const_type>
    {
        release_current();
        this->object_type::reset_id(other.object_type::reset_id());
        return *this;
    }

    // GLConst -> GLMutable converting move construction is forbidden.
    GLUnique(GLUnique<const_type>&&) noexcept
        requires std::same_as<object_type, mutable_type> = delete;

    // GLConst -> GLMutable converting move assignment is forbidden.
    GLUnique& operator=(GLUnique<const_type>&&) noexcept
        requires std::same_as<object_type, mutable_type> = delete;


    ~GLUnique() noexcept { release_current(); }

private:
    void release_current() noexcept {
        if (this->object_type::id()) {
            this->allocator_type::release(this->object_type::id());
        }
    }
};




/*
GLMutability
[mutability_tag, specifies_mutability]
    |
RawGLHandle
[has_basic_raw_handle_semantics]
    |
RawKindHandle        ------> GLAllocator<RawKindHandle>
[raw_gl_handle_type]         [allocatable_gl_handle]
    |                            |
RawObject(Handle)                |
[raw_gl_object_type]             |
    |                            |
GLUnique     <-------------------/
[unmanaged_gl_handle, unmanaged_gl_object]

*/




} // namespace josh
