#pragma once
#include "GLObjects.hpp"
#include "Size.hpp"
#include <glbinding/gl/gl.h>


namespace josh {


class RenderTargetDepthCubemapArray {
private:
    UniqueCubemapArray cubemaps_;
    UniqueFramebuffer fbo_;

    Size3I size_;

public:
    RenderTargetDepthCubemapArray(Size3I size)
        : size_{ size }
    {
        using namespace gl;

        cubemaps_.bind()
            .specify_all_images(size_,
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT }, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        fbo_.bind_draw()
            .attach_cubemap_array(cubemaps_, GL_DEPTH_ATTACHMENT)
            .set_draw_buffer(GL_NONE)
            .set_read_buffer(GL_NONE)
            .unbind();
    }


    UniqueCubemapArray& depth_taget() noexcept { return cubemaps_; }
    const UniqueCubemapArray& depth_taget() const noexcept { return cubemaps_; }

    UniqueFramebuffer& framebuffer() noexcept { return fbo_; }

    Size3I size() const noexcept { return size_; }

    void reset_size(Size3I new_size) {
        using namespace gl;

        size_ = new_size;

        cubemaps_.bind()
            .specify_all_images(size_,
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT }, nullptr)
            .unbind();
    }





};



} // namespace josh
