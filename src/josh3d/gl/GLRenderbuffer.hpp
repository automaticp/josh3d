#pragma once
#include "detail/AndThen.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp" // IWYU pragma: keep (concepts)
#include "GLKindHandles.hpp"
#include "GLTextures.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>


namespace josh {

template<mutability_tag MutT> class BoundRenderbuffer;
template<mutability_tag MutT> class RawRenderbuffer;



/*
We treat renderbuffers similar to textures for reflection with GLTexInfo.
This helps when using them as framebuffer attachments.
*/
struct TexSpecRB {
    GLenum  internal_format;
    GLsizei num_samples;
    TexSpecRB(GLenum internal_format, GLsizei num_samples = 1)
        : internal_format{ internal_format }, num_samples{ num_samples }
    {}
};

namespace detail {
template<> struct GLTexSpecImpl<gl::GL_RENDERBUFFER> { using type = TexSpecRB; };
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

    // TexSpecRB::num_samples is ignored here.
    // Ask ARB why the hell there's no separate
    // GL_RENDERBUFFER_MULTISAMPLE target instead.
    BoundRenderbuffer& allocate_storage(
        const Size2I& size, const TexSpecRB& spec)
        requires gl_mutable<MutT>
    {
        gl::glRenderbufferStorage(
            gl::GL_RENDERBUFFER, spec.internal_format,
            size.width, size.height
        );
        return *this;
    }

    BoundRenderbuffer& allocate_multisample_storage(
        const Size2I& size, const TexSpecRB& spec)
        requires gl_mutable<MutT>
    {
        gl::glRenderbufferStorageMultisample(
            gl::GL_RENDERBUFFER, spec.num_samples, spec.internal_format,
            size.width, size.height
        );
        return *this;
    }

    Size2I get_size() {
        using enum GLenum;
        GLint width, height;
        gl::glGetRenderbufferParameteriv(target_type, GL_RENDERBUFFER_WIDTH, &width);
        gl::glGetRenderbufferParameteriv(target_type, GL_RENDERBUFFER_HEIGHT, &height);
        return { width, height };
    }

    TexSpecRB get_spec() {
        using enum GLenum;
        GLenum internal_format;
        GLint nsamples;
        gl::glGetRenderbufferParameteriv(target_type, GL_RENDERBUFFER_INTERNAL_FORMAT, &internal_format);
        gl::glGetRenderbufferParameteriv(target_type, GL_RENDERBUFFER_SAMPLES, &nsamples);
        return { internal_format, nsamples };
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
