#pragma once
#include "detail/AndThen.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp" // IWYU pragma: keep (concepts)
#include "GLKindHandles.hpp"
#include "GLTextures.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace josh {

template<mutability_tag MutT> class BoundRenderbuffer;
template<mutability_tag MutT> class RawRenderbuffer;



/*
We treat renderbuffers similar to textures for reflection with GLTexInfo.
This helps when using them as framebuffer attachments.
*/
template<> struct GLTexSpec<gl::GL_RENDERBUFFER> {
    GLenum  internal_format;
    GLsizei num_samples;
    GLTexSpec(GLenum internal_format, GLsizei num_samples = 1)
        : internal_format{ internal_format }, num_samples{ num_samples }
    {}
};

namespace detail {
template<> struct GLTexSizeImpl<gl::GL_RENDERBUFFER> { using type = Size2I; };
} // namespace detail



template<mutability_tag MutT>
class BoundRenderbuffer
    : public detail::AndThen<BoundRenderbuffer<MutT>>
    , public detail::GLTexInfo<gl::GL_RENDERBUFFER>
{
private:
    friend RawRenderbuffer<MutT>;
    BoundRenderbuffer() = default;
public:
    static void unbind() { gl::glBindRenderbuffer(gl::GL_RENDERBUFFER, 0); }

    BoundRenderbuffer& create_storage(
        Size2I size, GLenum internal_format)
        requires gl_mutable<MutT>
    {
        gl::glRenderbufferStorage(
            gl::GL_RENDERBUFFER, internal_format,
            size.width, size.height
        );
        return *this;
    }

    BoundRenderbuffer& create_multisample_storage(
        Size2I size, GLsizei num_samples, GLenum internal_format)
        requires gl_mutable<MutT>
    {
        gl::glRenderbufferStorageMultisample(
            gl::GL_RENDERBUFFER, num_samples, internal_format,
            size.width, size.height
        );
        return *this;
    }

};




template<mutability_tag MutT>
class RawRenderbuffer
    : public RawRenderbufferHandle<MutT>
    , public detail::ObjectHandleTypeInfo<RawRenderbuffer, MutT>
    , public detail::GLTexInfo<gl::GL_RENDERBUFFER>
{
public:
    JOSH3D_MAGIC_CONSTRUCTORS(RawRenderbuffer, MutT, RawRenderbufferHandle<MutT>)

    BoundRenderbuffer<MutT> bind() const noexcept {
        gl::glBindRenderbuffer(gl::GL_RENDERBUFFER, this->id());
        return {};
    }
};




} // namespace josh
