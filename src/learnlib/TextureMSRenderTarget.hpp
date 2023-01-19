#pragma once

#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/types.h>


namespace learn {


class TextureMSRenderTarget {
private:
    TextureMS tex_;
    Framebuffer fbo_;
    Renderbuffer rbo_;

public:
    TextureMSRenderTarget(gl::GLsizei width, gl::GLsizei height, gl::GLsizei nsamples) {
        using namespace gl;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(width, height, nsamples);
        //  .set_parameter(...) ???

        rbo_.bind()
            .create_multisample_storage(width, height, nsamples, GL_DEPTH24_STENCIL8);

        fbo_.bind()
            .attach_multisample_texture(tex_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rbo_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();

    }

    Framebuffer& framebuffer() noexcept { return fbo_; }

    void reset_size_and_samples(gl::GLsizei width, gl::GLsizei height, gl::GLsizei nsamples) {
        using namespace gl;

        tex_.bind_to_unit(GL_TEXTURE0)
            .specify_image(width, height, nsamples);

        rbo_.bind()
            .create_multisample_storage(width, height, nsamples, GL_DEPTH24_STENCIL8);
    }


};



} // namespace learn


