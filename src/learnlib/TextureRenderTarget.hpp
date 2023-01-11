#pragma once

#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace learn {


class TextureRenderTarget {
private:
    TextureHandle tex_;
    Framebuffer fb_;
    Renderbuffer rb_;


public:
    TextureRenderTarget(gl::GLsizei width, gl::GLsizei height) {
        using namespace gl;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            // Fixes edge overflow from kernel effects
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        rb_ .bind()
            .create_storage(width, height, GL_DEPTH24_STENCIL8);

        fb_ .bind()
            .attach_texture(tex_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rb_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();

    }

    TextureHandle& target_texture() noexcept { return tex_; }
    const TextureHandle& target_texture() const noexcept { return tex_; }

    Framebuffer& framebuffer() noexcept { return fb_; }

    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        using namespace gl;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        rb_.bind().create_storage(width, height, GL_DEPTH24_STENCIL8);

    }


};



} // namespace learn
