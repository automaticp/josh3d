#pragma once
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>



namespace josh {


class RenderTargetColorAndDepth {
private:
    UniqueTexture2D color_;
    UniqueTexture2D depth_;
    UniqueFramebuffer fbo_;

    Size2I size_;

    UniqueTexture2D::spec_type spec_;

public:
    RenderTargetColorAndDepth(Size2I size)
        : RenderTargetColorAndDepth(size, gl::GL_RGBA, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE)
    {}

    RenderTargetColorAndDepth(Size2I size,
        GLenum color_format, GLenum color_internal_format,
        GLenum color_type)
        : size_{ size }
        , spec_{ color_internal_format, color_format, color_type }
    {
        using namespace gl;

        color_.bind()
            .specify_image(size_, spec_, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        depth_.bind()
            .specify_image(size_,
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT }, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        fbo_.bind_draw()
            .attach_texture(color_, GL_COLOR_ATTACHMENT0)
            .attach_texture(depth_, GL_DEPTH_ATTACHMENT)
            .unbind();

    }

    UniqueTexture2D& color_target() noexcept { return color_; }
    const UniqueTexture2D& color_target() const noexcept { return color_; }

    UniqueTexture2D& depth_target() noexcept { return depth_; }
    const UniqueTexture2D& depth_target() const noexcept { return depth_; }

    UniqueFramebuffer& framebuffer() noexcept { return fbo_; }

    Size2I size() const noexcept { return size_; }

    void reset_size(Size2I new_size) {
        using namespace gl;

        size_ = new_size;

        color_.bind()
            .specify_image(size_, spec_, nullptr);

        depth_.bind()
            .specify_image(size_,
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT }, nullptr);

    }


};


} // namespace josh
