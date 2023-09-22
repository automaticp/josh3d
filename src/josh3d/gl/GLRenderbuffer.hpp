#pragma once
#include "AndThen.hpp"
#include "RawGLHandles.hpp"
#include "Size.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp" // IWYU pragma: keep (concepts)
#include <glbinding/gl/gl.h>


namespace josh {

template<mutability_tag MutT> class BoundRenderbuffer;
template<mutability_tag MutT> class RawRenderbuffer;




template<mutability_tag MutT>
class BoundRenderbuffer
    : public detail::AndThen<BoundRenderbuffer<MutT>>
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
{
public:
    using RawRenderbufferHandle<MutT>::RawRenderbufferHandle;

    BoundRenderbuffer<MutT> bind() const noexcept {
        gl::glBindRenderbuffer(gl::GL_RENDERBUFFER, this->id());
        return {};
    }
};




} // namespace josh
