#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/gl.h>


namespace learn {


class RenderTargetDepthCubemapArray {
private:
    CubemapArray cubemaps_;
    Framebuffer fbo_;

    gl::GLsizei width_;
    gl::GLsizei height_;
    gl::GLsizei depth_;

public:
    RenderTargetDepthCubemapArray(gl::GLsizei width,
        gl::GLsizei height, gl::GLsizei depth)
        : width_{ width }
        , height_{ height }
        , depth_{ depth }
    {
        using namespace gl;

        cubemaps_.bind()
            .specify_all_images(
                width_, height_, depth_,
                GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT,
                nullptr
            )
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        fbo_.bind_draw()
            .attach_cubemap(cubemaps_, GL_DEPTH_ATTACHMENT)
            .and_then([] {
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            })
            .unbind();
    }


    CubemapArray& depth_taget() noexcept { return cubemaps_; }
    const CubemapArray& depth_taget() const noexcept { return cubemaps_; }

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
            .specify_all_images(width_, height_, depth_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .unbind();
    }





};



} // namespace learn
