#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/gl.h>



namespace learn {


class RenderTargetDepth {
private:
    TextureHandle tex_;
    Framebuffer fbo_;

    gl::GLsizei width_;
    gl::GLsizei height_;


public:
    RenderTargetDepth(gl::GLsizei width, gl::GLsizei height)
        : width_{ width }
        , height_{ height }
    {
        using namespace gl;

        tex_.bind()
            .specify_image(width_, height_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

        fbo_.bind()
            .attach_texture(tex_, GL_DEPTH_ATTACHMENT)
            .and_then([] {
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            })
            .unbind();

    }

    TextureHandle& target_texture() noexcept { return tex_; }
    const TextureHandle& target_texture() const noexcept { return tex_; }

    Framebuffer& framebuffer() noexcept { return fbo_; }

    gl::GLsizei width() const noexcept { return width_; }
    gl::GLsizei height() const noexcept { return height_; }

    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        using namespace gl;

        width_ = width;
        height_ = height;

        tex_.bind()
            .specify_image(width_, height_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .unbind();
    }


};





} // namespace learn
