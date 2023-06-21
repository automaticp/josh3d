#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/gl.h>


namespace learn {


class RenderTargetDepth {
private:
    Texture2D tex_;
    Framebuffer fbo_;

    gl::GLsizei width_;
    gl::GLsizei height_;


public:
    RenderTargetDepth(gl::GLsizei width, gl::GLsizei height)
        : width_{ width }
        , height_{ height }
    {
        using namespace gl;

        const float border_color[4] = { 1.f, 1.f, 1.f, 1.f };

        tex_.bind()
            .specify_image(width_, height_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
            // Maybe don't do this in the render target though?
            .set_parameter(GL_TEXTURE_BORDER_COLOR, border_color);

        fbo_.bind()
            .attach_texture(tex_, GL_DEPTH_ATTACHMENT)
            .and_then([] {
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            })
            .unbind();

    }

    Texture2D& depth_target() noexcept { return tex_; }
    const Texture2D& depth_target() const noexcept { return tex_; }

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
