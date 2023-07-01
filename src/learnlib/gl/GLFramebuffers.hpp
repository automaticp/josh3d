#pragma once
#include "GLObjectHandles.hpp"
#include "AndThen.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
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

class Framebuffer;
class BoundReadFramebuffer;




class BoundDrawFramebuffer : public detail::AndThen<BoundDrawFramebuffer> {
private:
    friend Framebuffer;
    BoundDrawFramebuffer() = default;

public:
    static void unbind() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); }

    // FIXME: Should accept Texture2D& and not just id
    // for proper type- and const- correctness.
    BoundDrawFramebuffer& attach_texture(GLuint texture,
        GLenum attachment, GLint mipmap_level = 0)
    {
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, mipmap_level);
        return *this;
    }

    BoundDrawFramebuffer& attach_multisample_texture(GLuint texture,
        GLenum attachment, GLint mipmap_level = 0)
    {
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, texture, mipmap_level);
        return *this;
    }

    BoundDrawFramebuffer& attach_renderbuffer(GLuint renderbuffer,
        GLenum attachment)
    {
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer);
        return *this;
    }

    BoundDrawFramebuffer& attach_cubemap(GLuint cubemap,
        GLenum attachment, GLint mipmap_level = 0)
    {
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, cubemap, mipmap_level);
        return *this;
    }

    BoundDrawFramebuffer& attach_texture_layer(GLuint texture,
        GLenum attachment, GLint layer, GLint mipmap_level = 0)
    {
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachment, texture, mipmap_level, layer);
        return *this;
    }

    BoundDrawFramebuffer& blit_from(const BoundReadFramebuffer& src,
        GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1,
        GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1,
        ClearBufferMask buffer_mask, GLenum interp_filter)
    {
        glBlitFramebuffer(
            src_x0, src_y0, src_x1, src_y1,
            dst_x0, dst_y0, dst_x1, dst_y1,
            buffer_mask, interp_filter
        );
        return *this;
    }

};




class BoundReadFramebuffer : public detail::AndThen<BoundReadFramebuffer> {
private:
    friend Framebuffer;
    BoundReadFramebuffer() = default;

public:
    static void unbind() { glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); }

    BoundReadFramebuffer& blit_to(BoundDrawFramebuffer& dst,
        GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1,
        GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1,
        ClearBufferMask buffer_mask, GLenum interp_filter)
    {
        glBlitFramebuffer(
            src_x0, src_y0, src_x1, src_y1,
            dst_x0, dst_y0, dst_x1, dst_y1,
            buffer_mask, interp_filter
        );
        return *this;
    }


};




class Framebuffer : public FramebufferHandle {
public:
    BoundDrawFramebuffer bind_draw() noexcept {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id_);
        return {};
    }

    BoundReadFramebuffer bind_read() const noexcept {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, id_);
        return {};
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

using leaksgl::BoundDrawFramebuffer, leaksgl::BoundReadFramebuffer, leaksgl::Framebuffer;
using leaksgl::BoundRenderbuffer, leaksgl::Renderbuffer;

} // namespace learn
