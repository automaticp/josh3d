#pragma once
#include "AndThen.hpp"
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLMutability.hpp"
#include "GLScalars.hpp"
#include "RawGLHandles.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>


// WIP

namespace josh {


template<mutability_tag MutT> class BoundReadFramebuffer;
template<mutability_tag MutT> class BoundDrawFramebuffer;
template<mutability_tag MutT> class RawFramebuffer;




template<mutability_tag MutT>
class BoundReadFramebuffer
    : public detail::AndThen<BoundReadFramebuffer<MutT>>
{
private:
    friend RawFramebuffer<MutT>;
    BoundReadFramebuffer() = default;
public:
    static void unbind() { gl::glBindFramebuffer(gl::GL_READ_FRAMEBUFFER, 0); }

    BoundReadFramebuffer& blit_to(
        const BoundDrawFramebuffer<GLMutable>& dst [[maybe_unused]],
        GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1,
        GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1,
        gl::ClearBufferMask buffer_mask, GLenum interp_filter)
    {
        gl::glBlitFramebuffer(
            src_x0, src_y0, src_x1, src_y1,
            dst_x0, dst_y0, dst_x1, dst_y1,
            buffer_mask, interp_filter
        );
        return *this;
    }

    BoundReadFramebuffer& set_read_buffer(GLenum attachment) const {
        gl::glReadBuffer(attachment);
        return *this;
    }

};


template<mutability_tag MutT>
class BoundDrawFramebuffer
    : public detail::AndThen<BoundDrawFramebuffer<MutT>>
{
private:
    friend RawFramebuffer<MutT>;
    BoundDrawFramebuffer() = default;
    static_assert(gl_mutable<MutT>, "Draw Framebuffer cannot be GLConst.");
public:
    static void unbind() { gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, 0); }

    BoundDrawFramebuffer& attach_texture(
        GLuint texture, GLenum attachment, GLint mipmap_level = 0)
    {
        gl::glFramebufferTexture2D(
            gl::GL_DRAW_FRAMEBUFFER, attachment,
            gl::GL_TEXTURE_2D, texture, mipmap_level
        );
        return *this;
    }

    BoundDrawFramebuffer& attach_multisample_texture(
        GLuint texture, GLenum attachment, GLint mipmap_level = 0)
    {
        gl::glFramebufferTexture2D(
            gl::GL_DRAW_FRAMEBUFFER, attachment,
            gl::GL_TEXTURE_2D_MULTISAMPLE, texture, mipmap_level
        );
        return *this;
    }

    BoundDrawFramebuffer& attach_renderbuffer(
        GLuint renderbuffer, GLenum attachment)
    {
        gl::glFramebufferRenderbuffer(
            gl::GL_DRAW_FRAMEBUFFER, attachment,
            gl::GL_RENDERBUFFER, renderbuffer
        );
        return *this;
    }

    BoundDrawFramebuffer& attach_texture_array(
        GLuint tex_array, GLenum attachment, GLint mipmap_level = 0) const
    {
        gl::glFramebufferTexture(
            gl::GL_DRAW_FRAMEBUFFER, attachment,
            tex_array, mipmap_level
        );
        return *this;
    }

    BoundDrawFramebuffer& attach_cubemap(
        GLuint cubemap, GLenum attachment, GLint mipmap_level = 0) const
    {
        gl::glFramebufferTexture(
            gl::GL_DRAW_FRAMEBUFFER, attachment,
            cubemap, mipmap_level
        );
        return *this;
    }

    BoundDrawFramebuffer& attach_texture_layer(
        GLuint texture, GLenum attachment, GLint layer,
        GLint mipmap_level = 0) const
    {
        gl::glFramebufferTextureLayer(
            gl::GL_DRAW_FRAMEBUFFER, attachment,
            texture, mipmap_level, layer
        );
        return *this;
    }

    template<mutability_tag MutU>
    BoundDrawFramebuffer& blit_from(
        const BoundReadFramebuffer<MutU>& src [[maybe_unused]],
        GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1,
        GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1,
        gl::ClearBufferMask buffer_mask, GLenum interp_filter) const
    {
        gl::glBlitFramebuffer(
            src_x0, src_y0, src_x1, src_y1,
            dst_x0, dst_y0, dst_x1, dst_y1,
            buffer_mask, interp_filter
        );
        return *this;
    }

    BoundDrawFramebuffer& set_draw_buffer(GLenum buffer_attachment) const {
        gl::glDrawBuffer(buffer_attachment);
        return *this;
    }

    template<std::same_as<GLenum> ...EnumT>
    BoundDrawFramebuffer& set_draw_buffers(EnumT... attachments) const {
        constexpr GLsizei n_slots = sizeof...(EnumT);
        GLenum slots[n_slots]{ attachments... };
        gl::glDrawBuffers(n_slots, slots);
        return *this;
    }

    BoundDrawFramebuffer& set_read_buffer(GLenum buffer_attachment) const {
        gl::glReadBuffer(buffer_attachment);
        return *this;
    }

};



template<mutability_tag MutT>
class RawFramebuffer
    : public RawFramebufferHandle<MutT>
    , public detail::ObjectHandleTypeInfo<RawFramebuffer, MutT>
{
public:
    using RawFramebufferHandle<MutT>::RawFramebufferHandle;

    BoundDrawFramebuffer<MutT> bind_draw() const noexcept
        requires gl_mutable<typename RawFramebuffer::mutability_type>
    {
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, this->id());
        return {};
    }

    BoundReadFramebuffer<MutT> bind_read() const noexcept {
        gl::glBindFramebuffer(gl::GL_READ_FRAMEBUFFER, this->id());
        return {};
    }

};




} // namespace josh
