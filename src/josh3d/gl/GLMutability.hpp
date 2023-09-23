#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include <concepts>



namespace josh {

/*
In order to model mutability of OpenGL *objects* we need to effectively
have handle-to-const and handle-to-non-const type variations.

In C++, there's a pointer-to-const and pointer-to-non-const, which
do exactly that for pointed-to objects. This, however *feature* is
exclusive to pointer and reference types.

For custom handles like OpenGL object ID, this cannot be achieved with
the `const` qualifier. `const Handle` would be equivalent to const-pointer,
and not pointer-to-const.

From the perspective of the C++ type system, we do, indeed, have to create
two different types of Handles depending on the mutability of the underlying
objects. And then recreate semantics of mutable->const conversions, similar
to how `T*` can bind to `const T*`, but not the other way around.

In this implementation, this is handled with tag types GLConst and GLMutable,
that are supplied as a template parameter for each Handle type.

Below is a simple table showing how each Raw Handle type can be thought of
as a pointer to an OpenGL object:

Opaque Handle:
    RawGLHandle<GLMutable>      - void*
    RawGLHandle<GLConst>        - const void*

Kind-Handle:
    RawTextureHandle<GLMutable> - TextureKind*
    RawTextureHandle<GLConst>   - const TextureKind*

Object-Handle:
    RawTexture2D<GLMutable>     - Texture2D*
    RawTexture2D<GLConst>       - const Texture2D*


This also has implications on the meaning of `const` qualifiers when passing
Handles as function parameters. The Handles should almost never be passed as:

    void fun(const RawTexture2D<GLMutable>& handle);

because this is semantically equivalent to passing:

    void fun(Texture2D* const handle);

meaning it's the value of the Handle itself that is const, but the underlying object
is still modifyable through that Handle. This is probably not what the author intended.

Instead, the Raw Handle types should be passed by value (sizeof(GLuint) == 4 bytes),
and should communicate mutability through template parameters:

    void fun(RawTexture2D<GLConst> handle);

Due to GLMutable->GLConst converting constructors, this function can be called on
both `RawTexture2D<GLConst>` and `RawTexture2D<GLMutable>` handles. Same is true
for returning handles from functions:

    class Example {
    private:
        RawTexture2D<GLMutable> tex_;
    public:
        RawTexture2D<GLConst> get_texture_for_reading() const {
            return tex_;
        }
    };

Or, if you want to propagate const for an accessor:

    class Example2 {
    private:
        RawTexture2D<GLMutable> tex_;
    public:
        RawTexture2D<GLMutable> texture()       { return tex_; }
        RawTexture2D<GLConst>   texture() const { return tex_; }
    };

Again, it's all very similar to how pointers and references already behave.
*/


/*
On the topic of what would make sense to actually consider to be a const operation:

1. Modification of a property of an OpenGL object specifically: writing to/resizing buffers,
changing draw hints, parameters, etc. - is a non-const operation.

2. Operation that modifies an OpenGL context but not the properties of objects: binding,
changing active units, buffer bindnigs, etc. - *can* be considered a const operation.

3. Read operation on an object: getting properties, validation, etc - is a const operation.

The second point is probably the most important one, as without it, you can't really do anything
useful and still preserve some sense of const-correctness. If I can't even bind a texture
for sampling (reading) when it's GLConst, then that const handle is useless for me.
*/



/*
Mutability tag used for specifying that the referenced OpenGL object
cannot be modified through this handle. Models pointer-to-const.
*/
struct GLConst   { using mutability_type = GLConst;   };

/*
Mutability tag used for specifying that the referenced OpenGL object
can be modified through this handle. Models pointer-to-non-const.
*/
struct GLMutable { using mutability_type = GLMutable; };




template<typename MutabilityT>
concept mutability_tag = any_of<MutabilityT, GLConst, GLMutable>;


template<typename MutT>
concept gl_const = std::same_as<MutT, GLConst>;

template<typename MutT>
concept gl_mutable = std::same_as<MutT, GLMutable>;


template<typename GLHandleT>
concept specifies_mutability =
    (std::derived_from<GLHandleT, GLConst>   && gl_const<typename GLHandleT::mutability_type>  ) ||
    (std::derived_from<GLHandleT, GLMutable> && gl_mutable<typename GLHandleT::mutability_type>);


namespace detail {

template<mutability_tag MutT>
struct OppositeGLMutabilityImpl;

template<> struct OppositeGLMutabilityImpl<GLMutable> { using type = GLConst;   };
template<> struct OppositeGLMutabilityImpl<GLConst>   { using type = GLMutable; };

} // namespace detail


template<mutability_tag MutT>
using OppositeGLMutability = detail::OppositeGLMutabilityImpl<MutT>::type;


} // namespace josh
