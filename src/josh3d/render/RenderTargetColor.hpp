#pragma once
#include "GLScalars.hpp"
#include "GLObjects.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace josh {


class RenderTargetColor {
private:
    Texture2D tex_;
    Framebuffer fb_;
    Renderbuffer rb_;

    Size2I size_;

    GLenum color_format_;
    GLenum color_internal_format_;
    GLenum color_type_;

public:
    RenderTargetColor(Size2I size)
        : RenderTargetColor(size, gl::GL_RGBA, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE)
    {}

    RenderTargetColor(Size2I size,
        GLenum color_format, GLenum color_internal_format,
        GLenum color_type
    )
        : size_{ size }
        , color_format_{ color_format }
        , color_internal_format_{ color_internal_format }
        , color_type_{ color_type }
    {
        using namespace gl;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(size_, color_internal_format_, color_format_, color_type_, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            // Fixes edge overflow from kernel effects
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        rb_ .bind()
            .create_storage(size_, GL_DEPTH24_STENCIL8);

        fb_ .bind_draw()
            .attach_texture(tex_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rb_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();

    }

    Texture2D& color_target() noexcept { return tex_; }
    const Texture2D& color_target() const noexcept { return tex_; }

    Framebuffer& framebuffer() noexcept { return fb_; }

    Size2I size() const noexcept { return size_; }

    void reset_size(Size2I new_size) {
        using namespace gl;

        size_ = new_size;

        tex_.bind()
            .specify_image(size_, color_internal_format_,
                color_format_, color_type_, nullptr);

        rb_.bind().create_storage(size_, GL_DEPTH24_STENCIL8);

    }


};



} // namespace josh

