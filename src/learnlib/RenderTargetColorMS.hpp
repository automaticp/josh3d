#pragma once

#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/types.h>


namespace learn {


class RenderTargetColorMS {
private:
    TextureMS tex_;
    Framebuffer fbo_;
    Renderbuffer rbo_;

    gl::GLsizei width_;
    gl::GLsizei height_;
    gl::GLsizei nsamples_;


public:
    RenderTargetColorMS(gl::GLsizei width, gl::GLsizei height, gl::GLsizei nsamples)
        : width_{ width }
        , height_{ height }
        , nsamples_{ nsamples }
    {
        using namespace gl;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(width_, height_, nsamples_);
        //  .set_parameter(...) ???

        rbo_.bind()
            .create_multisample_storage(width_, height_, nsamples_, GL_DEPTH24_STENCIL8);

        fbo_.bind()
            .attach_multisample_texture(tex_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rbo_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();

    }

    TextureMS& color_target() noexcept { return tex_; }
    const TextureMS& color_target() const noexcept { return tex_; }

    Framebuffer& framebuffer() noexcept { return fbo_; }

    gl::GLsizei width() const noexcept { return width_; }
    gl::GLsizei height() const noexcept { return height_; }
    gl::GLsizei nsamples() const noexcept { return nsamples_; }

    void reset_size_and_samples(gl::GLsizei width, gl::GLsizei height, gl::GLsizei nsamples) {
        using namespace gl;

        width_ = width;
        height_ = height;
        nsamples_ = nsamples;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(width_, height_, nsamples_);

        rbo_.bind()
            .create_multisample_storage(width_, height_, nsamples_, GL_DEPTH24_STENCIL8);
    }


};



} // namespace learn


