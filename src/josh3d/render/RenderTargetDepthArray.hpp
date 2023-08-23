#pragma once
#include "GLFramebuffers.hpp"
#include "GLScalars.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "GlobalsUtil.hpp"
#include "Size.hpp"
#include <glbinding-aux/Meta.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>


namespace josh {


class RenderTargetDepthArray {
private:
    Texture2DArray tex_;
    Framebuffer fbo_;

    Size3I size_;

    GLenum type_;

public:
    RenderTargetDepthArray(Size3I size, GLenum type = gl::GL_FLOAT)
        : size_{ size }
        , type_{ type }
    {
        using enum GLenum;

        const float border_color[4]{ 1.f, 1.f, 1.f, 1.f };

        tex_.bind()
            .specify_all_images(size_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, type_, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_BORDER_COLOR, border_color);

        fbo_.bind_draw()
            .attach_texture_array(tex_, GL_DEPTH_ATTACHMENT)
            .set_draw_buffer(GL_NONE)
            .set_read_buffer(GL_NONE)
            .unbind();

    }

    Texture2DArray& depth_target() noexcept { return tex_; }
    const Texture2DArray& depth_target() const noexcept { return tex_;  }

    Framebuffer& framebuffer() noexcept { return fbo_; }
    const Framebuffer& framebuffer() const noexcept { return fbo_; }

    Size3I size() const noexcept { return size_; }
    GLenum type() const noexcept { return type_; }

    void reset_size(Size3I new_size) {
        using enum GLenum;

        size_ = new_size;

        tex_.bind()
            .specify_all_images(size_,
                GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, type_, nullptr)
            .unbind();

    }


};


} // namespace josh
