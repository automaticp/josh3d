#pragma once
#include "GLScalars.hpp"
#include "GLObjects.hpp"
#include "Size.hpp"
#include <glbinding/gl/gl.h>


namespace josh {

class RenderTargetDepthCubemap {
private:
    Cubemap cubemap_;
    Framebuffer fbo_;

    Size2I size_;

public:
    RenderTargetDepthCubemap(Size2I size)
        : size_{ size }
    {
        using namespace gl;

        cubemap_.bind()
            .specify_all_images(size_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
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

    Size2I size() const noexcept { return size_; }

    void reset_size(Size2I new_size) {
        using enum GLenum;

        size_ = new_size;

        cubemap_.bind()
            .specify_all_images(size_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr)
            .unbind();
    }

};

} // namespace josh
