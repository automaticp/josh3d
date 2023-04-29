#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace learn {


class RenderTargetColor {
private:
    TextureHandle tex_;
    Framebuffer fb_;
    Renderbuffer rb_;

    gl::GLsizei width_;
    gl::GLsizei height_;

    gl::GLenum color_format_{ gl::GL_RGBA };
    gl::GLenum color_internal_format_{ gl::GL_RGBA };
    gl::GLenum color_type_{ gl::GL_UNSIGNED_BYTE };

public:
    RenderTargetColor(gl::GLsizei width, gl::GLsizei height)
        : RenderTargetColor(width, height, gl::GL_RGBA, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE)
    {}

    RenderTargetColor(gl::GLsizei width, gl::GLsizei height,
        gl::GLenum color_format, gl::GLenum color_internal_format,
        gl::GLenum color_type
    )
        : width_{ width }
        , height_{ height }
        , color_format_{ color_format }
        , color_internal_format_{ color_internal_format }
        , color_type_{ color_type }
    {
        using namespace gl;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(width_, height_, color_internal_format_, color_format_, color_type_, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            // Fixes edge overflow from kernel effects
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        rb_ .bind()
            .create_storage(width_, height_, GL_DEPTH24_STENCIL8);

        fb_ .bind()
            .attach_texture(tex_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rb_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();

    }

    TextureHandle& color_target() noexcept { return tex_; }
    const TextureHandle& color_target() const noexcept { return tex_; }

    Framebuffer& framebuffer() noexcept { return fb_; }

    gl::GLsizei width() const noexcept { return width_; }
    gl::GLsizei height() const noexcept { return height_; }

    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        using namespace gl;

        width_ = width;
        height_ = height;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(
                width_, height_, color_internal_format_,
                color_format_, color_type_, nullptr
            );

        rb_.bind().create_storage(width_, height_, GL_DEPTH24_STENCIL8);

    }


};



} // namespace learn

