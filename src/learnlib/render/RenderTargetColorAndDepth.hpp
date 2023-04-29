#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>



namespace learn {


class RenderTargetColorAndDepth {
private:
    TextureHandle color_;
    TextureHandle depth_;
    Framebuffer fbo_;

    gl::GLsizei width_;
    gl::GLsizei height_;

    gl::GLenum color_format_{ gl::GL_RGBA };
    gl::GLenum color_internal_format_{ gl::GL_RGBA };
    gl::GLenum color_type_{ gl::GL_UNSIGNED_BYTE };

public:
    RenderTargetColorAndDepth(gl::GLsizei width, gl::GLsizei height)
        : RenderTargetColorAndDepth(width, height, gl::GL_RGBA, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE)
    {}

    RenderTargetColorAndDepth(gl::GLsizei width, gl::GLsizei height,
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

        color_.bind()
            .specify_image(width_, height_, color_internal_format_,
                color_format_, color_type_, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        depth_.bind()
            .specify_image(width_, height_, GL_DEPTH_COMPONENT,
                GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        fbo_.bind()
            .attach_texture(color_, GL_COLOR_ATTACHMENT0)
            .attach_texture(depth_, GL_DEPTH_ATTACHMENT)
            .unbind();

    }

    TextureHandle& color_target() noexcept { return color_; }
    const TextureHandle& color_target() const noexcept { return color_; }

    TextureHandle& depth_target() noexcept { return depth_; }
    const TextureHandle& depth_target() const noexcept { return depth_; }

    Framebuffer& framebuffer() noexcept { return fbo_; }

    gl::GLsizei width() const noexcept { return width_; }
    gl::GLsizei height() const noexcept { return height_; }


    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        using namespace gl;

        width_ = width;
        height_ = height;

        color_.bind()
            .specify_image(width_, height_, color_internal_format_,
                color_format_, color_type_, nullptr);

        depth_.bind()
            .specify_image(width_, height_, GL_DEPTH_COMPONENT,
                GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    }


};


} // namespace learn
