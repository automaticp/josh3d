#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/gl.h>


namespace josh {

class RenderTargetDepthCubemap {
private:
    Cubemap cubemap_;
    Framebuffer fbo_;

    gl::GLsizei width_;
    gl::GLsizei height_;

public:
    RenderTargetDepthCubemap(gl::GLsizei width, gl::GLsizei height)
        : width_{ width }
        , height_{ height }
    {
        using namespace gl;

        cubemap_.bind()
            .specify_all_images(width_, height_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        fbo_.bind_draw()
            .attach_cubemap(cubemap_, GL_DEPTH_ATTACHMENT)
            .set_draw_buffer(GL_NONE)
            .set_read_buffer(GL_NONE)
            .unbind();

    }

    Cubemap& depth_taget() noexcept { return cubemap_; }
    const Cubemap& depth_taget() const noexcept { return cubemap_; }

    Framebuffer& framebuffer() noexcept { return fbo_; }

    gl::GLsizei width() const noexcept { return width_; }
    gl::GLsizei height() const noexcept { return height_; }

    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        using namespace gl;

        width_ = width;
        height_ = height;

        cubemap_.bind()
            .specify_all_images(width_, height_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .unbind();
    }

};

} // namespace josh
