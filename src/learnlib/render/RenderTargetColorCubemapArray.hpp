#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace learn {


class RenderTargetColorCubemapArray {
private:
    CubemapArray cubemaps_;
    Renderbuffer rbo_;
    Framebuffer fbo_;

    gl::GLsizei width_;
    gl::GLsizei height_;
    gl::GLsizei depth_;

    gl::GLenum format_{ gl::GL_RGBA };
    gl::GLenum internal_format_{ gl::GL_RGBA };
    gl::GLenum type_{ gl::GL_UNSIGNED_BYTE };

public:
    RenderTargetColorCubemapArray(gl::GLsizei width, gl::GLsizei height, gl::GLsizei depth)
        : RenderTargetColorCubemapArray(
            width, height, depth,
            gl::GL_RGBA, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE
        )
    {}

    RenderTargetColorCubemapArray(
        gl::GLsizei width, gl::GLsizei height, gl::GLsizei depth,
        gl::GLenum format, gl::GLenum internal_format, gl::GLenum type
    )
        : width_{ width }
        , height_{ height }
        , depth_{ depth }
        , format_{ format }
        , internal_format_{ internal_format }
        , type_{ type }
    {
        using namespace gl;

        cubemaps_.bind()
            .specify_all_images(
                width_, height_, depth_,
                internal_format_, format_, type_,
                nullptr
            )
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        rbo_.bind()
            .create_storage(width_, height_, GL_DEPTH24_STENCIL8);

        fbo_.bind()
            .attach_texture(cubemaps_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rbo_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();
    }


    CubemapArray& color_taget() noexcept { return cubemaps_; }
    const CubemapArray& color_taget() const noexcept { return cubemaps_; }

    Framebuffer& framebuffer() noexcept { return fbo_; }

    gl::GLsizei width() const noexcept { return width_; }
    gl::GLsizei height() const noexcept { return height_; }
    gl::GLsizei depth() const noexcept { return depth_; }

    void reset_size(gl::GLsizei width, gl::GLsizei height, gl::GLsizei depth) {
        using namespace gl;

        width_ = width;
        height_ = height;
        depth_ = depth;

        cubemaps_.bind()
            .specify_all_images(width_, height_, depth_, internal_format_, format_, type_, nullptr)
            .unbind();
    }





};



} // namespace learn
