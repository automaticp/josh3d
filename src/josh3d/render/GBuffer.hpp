#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "GLTextures.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace josh {




class GBuffer {
private:
    Framebuffer fb_;
    Texture2D position_draw_;
    Texture2D normals_;
    Texture2D albedo_spec_;
    Renderbuffer depth_;

    GLsizei width_;
    GLsizei height_;

public:
    GBuffer(GLsizei width, GLsizei height)
        : width_{ width }
        , height_{ height }
    {
        using enum gl::GLenum;

        position_draw_.bind()
            .specify_image(width_, height_, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        normals_.bind()
            .specify_image(width_, height_, GL_RGBA8, GL_RGBA, GL_FLOAT, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        albedo_spec_.bind()
            .specify_image(width_, height_, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        depth_.bind()
            .create_storage(width_, height_, GL_DEPTH_COMPONENT);

        fb_.bind_draw()
            .attach_texture(position_draw_, GL_COLOR_ATTACHMENT0)
            .attach_texture(normals_,       GL_COLOR_ATTACHMENT1)
            .attach_texture(albedo_spec_,   GL_COLOR_ATTACHMENT2)
            .set_draw_buffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2)
            .attach_renderbuffer(depth_,    GL_DEPTH_ATTACHMENT);

    }

    void reattach_default_depth_stencil_buffer() {
        using enum GLenum;
        fb_.bind_draw().attach_renderbuffer(depth_, GL_DEPTH_ATTACHMENT);
    }

    // If the sizes do not match I don't care what happens to you.
    // Only supports pure depth attachments, no mixed depth/stencil.
    void attach_external_depth_buffer(Texture2D& depth) {
        using enum GLenum;
        fb_.bind_draw().attach_texture(depth, GL_DEPTH_ATTACHMENT);
    }

    // If the sizes do not match I don't care what happens to you.
    // Only supports pure depth attachments, no mixed depth/stencil.
    void attach_external_depth_buffer(Renderbuffer& depth) {
        using enum GLenum;
        fb_.bind_draw().attach_renderbuffer(depth, GL_DEPTH_ATTACHMENT);
    }


    const Texture2D& position_target() const noexcept { return position_draw_; }
    const Texture2D& normals_target() const noexcept { return normals_; }
    const Texture2D& albedo_spec_target() const noexcept { return albedo_spec_; }

    Framebuffer& framebuffer() noexcept { return fb_; }
    const Framebuffer& framebuffer() const noexcept { return fb_; }

    GLsizei width() const noexcept { return width_; }
    GLsizei height() const noexcept { return height_; }

    void reset_size(GLsizei width, GLsizei height) {
        using enum gl::GLenum;

        width_ = width;
        height_ = height;

        position_draw_.bind()
            .specify_image(width_, height_, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);

        normals_.bind()
            .specify_image(width_, height_, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);

        albedo_spec_.bind()
            .specify_image(width_, height_, GL_RGBA32F, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        depth_.bind()
            .create_storage(width_, height_, GL_DEPTH24_STENCIL8);

    }

};



} // namespace josh
