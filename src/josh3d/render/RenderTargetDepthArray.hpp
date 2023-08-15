#pragma once
#include "GLFramebuffers.hpp"
#include "GLScalars.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "GlobalsUtil.hpp"
#include <glbinding-aux/Meta.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>


namespace josh {


class RenderTargetDepthArray {
private:
    Texture2DArray tex_;
    Framebuffer fbo_;

    GLsizei width_;
    GLsizei height_;
    GLsizei layers_;
    GLenum  type_;

public:
    RenderTargetDepthArray(GLsizei width, GLsizei height,
        GLsizei layers, GLenum type = gl::GL_FLOAT)
        : width_{ width }
        , height_{ height }
        , layers_{ layers }
        , type_{ type }
    {
        using enum GLenum;

        const float border_color[4]{ 1.f, 1.f, 1.f, 1.f };

        tex_.bind()
            .specify_all_images(width_, height_, layers_, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, type_, nullptr)
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

    GLsizei width()  const noexcept { return width_;  }
    GLsizei height() const noexcept { return height_; }
    GLsizei layers() const noexcept { return layers_; }
    GLenum  type()   const noexcept { return type_;   }

    void reset_size(GLsizei width, GLsizei height, GLsizei layers) {
        using enum GLenum;

        width_  = width;
        height_ = height;
        layers_ = layers;

        tex_.bind()
            .specify_all_images(width_, height_, layers_,
                GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, type_, nullptr)
            .unbind();

    }


};


} // namespace josh
