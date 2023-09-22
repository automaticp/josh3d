#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include <concepts>



namespace josh {

/*
So it seems like we need a pointer and pointer-to-const versions of each type...
RawTexture2D still models a pointer;
const RawTexture2D is just a const-pointer, not pointer-to-const.
So we need a different way of modelling pointer-to-const.

RawGLHandle<GLMutable>      - void*
RawGLHandle<GLConst>        - const void*

RawTextureHandle<GLMutable> - TextureKind*
RawTextureHandle<GLConst>   - const TextureKind*

RawTexture2D<GLMutable>     - Texture2D*
RawTexture2D<GLConst>       - const Texture2D*

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
