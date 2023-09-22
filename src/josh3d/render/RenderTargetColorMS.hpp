#pragma once
#include "GLScalars.hpp"
#include "GLObjects.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/types.h>


namespace josh {


class RenderTargetColorMS {
private:
    Texture2DMS tex_;
    Framebuffer fbo_;
    Renderbuffer rbo_;

    Size2I size_;

    Texture2DMS::spec_type spec_;

public:
    RenderTargetColorMS(Size2I size,
        GLsizei nsamples, GLenum color_internal_format = gl::GL_RGBA)
        : size_{ size }
        , spec_{ color_internal_format, nsamples, false }
    {
        using namespace gl;

        tex_.bind()
            .specify_image(size_, spec_);
        //  .set_parameter(...) ???

        rbo_.bind()
            .create_multisample_storage(size_, spec_.nsamples, GL_DEPTH24_STENCIL8);

        fbo_.bind_draw()
            .attach_multisample_texture(tex_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rbo_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();

    }

    Texture2DMS& color_target() noexcept { return tex_; }
    const Texture2DMS& color_target() const noexcept { return tex_; }

    Framebuffer& framebuffer() noexcept { return fbo_; }

    Size2I  size()     const noexcept { return size_; }
    GLsizei nsamples() const noexcept { return spec_.nsamples; }

    void reset_size_and_samples(Size2I new_size, GLsizei nsamples) {
        using namespace gl;

        size_          = new_size;
        spec_.nsamples = nsamples;

        tex_.bind()
            .specify_image(size_, spec_);

        rbo_.bind()
            .create_multisample_storage(size_, spec_.nsamples, GL_DEPTH24_STENCIL8);
    }


};



} // namespace josh


