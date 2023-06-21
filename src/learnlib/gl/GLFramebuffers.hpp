#pragma once
#include "GLObjectHandles.hpp"
#include "AndThen.hpp"
#include <glbinding/gl/gl.h>





namespace learn {


/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/

namespace leaksgl {

using namespace gl;











class BoundFramebuffer : public detail::AndThen<BoundFramebuffer> {
private:
    GLenum target_;

    friend class Framebuffer;
    BoundFramebuffer(GLenum target) : target_{ target } {};

public:
    void unbind() {
        glBindFramebuffer(target_, 0);
    }

    static void unbind_as(GLenum target) {
        glBindFramebuffer(target, 0);
    }

    BoundFramebuffer& attach_texture(GLuint texture,
        GLenum attachment, GLint mipmap_level = 0)
    {
        glFramebufferTexture2D(target_, attachment, GL_TEXTURE_2D, texture, mipmap_level);
        return *this;
    }

    BoundFramebuffer& attach_multisample_texture(GLuint texture,
        GLenum attachment, GLint mipmap_level = 0)
    {
        glFramebufferTexture2D(target_, attachment, GL_TEXTURE_2D_MULTISAMPLE, texture, mipmap_level);
        return *this;
    }

    BoundFramebuffer& attach_renderbuffer(GLuint renderbuffer,
        GLenum attachment)
    {
        glFramebufferRenderbuffer(target_, attachment, GL_RENDERBUFFER, renderbuffer);
        return *this;
    }

    BoundFramebuffer& attach_cubemap(GLuint cubemap,
        GLenum attachment, GLint mipmap_level = 0)
    {
        glFramebufferTexture(target_, attachment, cubemap, mipmap_level);
        return *this;
    }

    BoundFramebuffer& attach_texture_layer(GLuint texture,
        GLenum attachment, GLint layer, GLint mipmap_level = 0)
    {
        glFramebufferTextureLayer(target_, attachment, texture, mipmap_level, layer);
        return *this;
    }

    // FIXME: might be a good idea to make 2 separate child classes for
    // DRAW and READ framebuffers and establish a blit_to() and blit_from()
    // methods with clear DRAW/READ bound state dependency. Something like:
    //
    // ... BoundReadFramebuffer::blit_to(BoundDrawFramebuffer& dst, ...);
    //
    BoundFramebuffer& blit(
        GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1,
        GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1,
        ClearBufferMask buffer_mask, GLenum interp_filter)
    {
        glBlitFramebuffer(
            src_x0, src_y0, src_x1, src_y1, dst_x0, dst_y0, dst_x1, dst_y1,
            buffer_mask, interp_filter
        );
        return *this;
    }
};


class Framebuffer : public FramebufferHandle {
public:
    BoundFramebuffer bind() {
        return bind_as(GL_FRAMEBUFFER);
    }

    BoundFramebuffer bind_as(GLenum target) {
        glBindFramebuffer(target, id_);
        return { target };
    }
};











class BoundRenderbuffer : public detail::AndThen<BoundRenderbuffer> {
private:
    friend class Renderbuffer;
    BoundRenderbuffer() = default;

public:
    BoundRenderbuffer& create_storage(GLsizei width, GLsizei height,
        GLenum internal_format)
    {
        glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
        return *this;
    }

    BoundRenderbuffer& create_multisample_storage(GLsizei width,
        GLsizei height, GLsizei samples, GLenum internal_format)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internal_format, width, height);
        return *this;
    }

    static void unbind() {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
};



class Renderbuffer : public RenderbufferHandle {
public:
    BoundRenderbuffer bind() {
        glBindRenderbuffer(GL_RENDERBUFFER, id_);
        return {};
    }

};





} // namespace leaksgl

using leaksgl::BoundFramebuffer, leaksgl::Framebuffer;
using leaksgl::BoundRenderbuffer, leaksgl::Renderbuffer;

} // namespace learn
