#pragma once
#include "GLScalars.hpp"
#include "GLObjects.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace josh {


class RenderTargetColorCubemapArray {
private:
    UniqueCubemapArray cubemaps_;
    UniqueRenderbuffer rbo_;
    UniqueFramebuffer fbo_;

    Size3I size_;

    UniqueCubemapArray::spec_type spec_;

public:
    RenderTargetColorCubemapArray(Size3I size)
        : RenderTargetColorCubemapArray{
            size, gl::GL_RGBA, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE
        }
    {}

    RenderTargetColorCubemapArray(Size3I size,
        GLenum format, GLenum internal_format, GLenum type)
        : size_{ size }
        , spec_{ internal_format, format, type }
    {
        using namespace gl;

        cubemaps_.bind()
            .specify_all_images(size_, spec_, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        rbo_.bind()
            .create_storage(Size2I{ size_ }, GL_DEPTH24_STENCIL8);

        fbo_.bind_draw()
            .attach_cubemap_array(cubemaps_, GL_COLOR_ATTACHMENT0)
            .attach_renderbuffer(rbo_, GL_DEPTH_STENCIL_ATTACHMENT)
            .unbind();
    }


    UniqueCubemapArray& color_taget() noexcept { return cubemaps_; }
    const UniqueCubemapArray& color_taget() const noexcept { return cubemaps_; }

    UniqueFramebuffer& framebuffer() noexcept { return fbo_; }

    Size3I size() const noexcept { return size_; }

    void reset_size(Size3I new_size) {
        using namespace gl;

        size_ = new_size;

        cubemaps_.bind()
            .specify_all_images(size_, spec_, nullptr)
            .unbind();
    }





};



} // namespace josh
