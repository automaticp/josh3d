#pragma once
#include "GLObjects.hpp"
#include "Size.hpp"
#include <glbinding/gl/gl.h>


namespace josh {


class RenderTargetDepth {
private:
    UniqueTexture2D tex_;
    UniqueFramebuffer fbo_;

    Size2I size_;

public:
    RenderTargetDepth(Size2I size)
        : size_{ size }
    {
        using namespace gl;

        const float border_color[4] = { 1.f, 1.f, 1.f, 1.f };

        tex_.bind()
            .specify_image(size_,
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT }, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
            // Maybe don't do this in the render target though?
            .set_parameter(GL_TEXTURE_BORDER_COLOR, border_color);

        fbo_.bind_draw()
            .attach_texture(tex_, GL_DEPTH_ATTACHMENT)
            .set_draw_buffer(GL_NONE)
            .set_read_buffer(GL_NONE)
            .unbind();

    }

    UniqueTexture2D& depth_target() noexcept { return tex_; }
    const UniqueTexture2D& depth_target() const noexcept { return tex_; }

    UniqueFramebuffer& framebuffer() noexcept { return fbo_; }

    Size2I size() const noexcept { return size_; }

    void reset_size(Size2I new_size) {
        using namespace gl;

        size_ = new_size;

        tex_.bind()
            .specify_image(size_,
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT }, nullptr)
            .unbind();
    }


};





} // namespace josh
