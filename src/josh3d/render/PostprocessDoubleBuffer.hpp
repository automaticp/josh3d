#pragma once
#include "GLScalars.hpp"
#include "RenderTargetColor.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <utility>
#include <glbinding/gl/types.h>


namespace josh {


/*
A swappable pair of buffers for sequentially overlaying
postprocessing effects on top of one another.

General usage instructions:
    1. Bind the backbuffer as a DRAW buffer;
    2. Draw the scene (sample from a front buffer or a previous target);
    3. Unbind the backbuffer;
    4. Swap the back and front buffers.

Stick to the Bind-Draw-Unbind-Swap order of operations.
The front buffer will contain the results ready for display.

Usage example:

    RenderTargetColor primary_target{ width, height };
    PostprocessDoubleBuffer pdb{ width, height };

    ...

    // Render the scene to some kind of primary buffer

    primary_target.framebuffer()
        .bind_draw()
        .and_then([&] {

            // Draw the scene here
            draw_scene_objects();

        })
        .unbind();

    // Then blit to the backbuffer of the PDB
    // with the Bind-Draw-Unbind-Swap idiom.

    pdb.back().framebuffer()
        .bind_draw()
        .and_then([&] {
            primary_target
                .bind_read()
                .blit(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST)
                .unbind();
        })
        .unbind();

    pdb.swap_buffers()

    // Do the double-buffered postprocessing using PDB
    // with the Bind-Draw-Unbind-Swap loop

    for (auto&& pp : std::span(pp_stages.begin(), pp_stages.end() - 1)) {
        pdb.back().framebuffer()
            .bind_draw()
            .and_then([&] {
                pp.draw(pdb.front_target());
            })
            .unbind();
        pdb.swap_buffers();
    }

    // Render last stage to the default framebuffer
    pp_stages.back().draw(pdb_.front_target());


Alternatively, if the primary buffer is not special in any way,
you can render the scene into the backbuffer of the PDB directly,
without having a separate primary buffer.

*/


class PostprocessDoubleBuffer {
private:
    RenderTargetColor buf1_;
    RenderTargetColor buf2_;

    RenderTargetColor* front_{ &buf1_ };
    RenderTargetColor* back_ { &buf2_ };

public:
    PostprocessDoubleBuffer(Size2I canvas_size)
        : PostprocessDoubleBuffer(canvas_size, gl::GL_RGBA, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE)
    {}

    PostprocessDoubleBuffer(Size2I canvas_size,
        GLenum color_format, GLenum color_internal_format,
        GLenum color_type
    )
        : buf1_{ canvas_size, color_format, color_internal_format, color_type }
        , buf2_{ canvas_size, color_format, color_internal_format, color_type }
    {}

    UniqueTexture2D& front_target() noexcept { return front_->color_target(); }
    const UniqueTexture2D& front_target() const noexcept { return front_->color_target(); }


    RenderTargetColor& front() noexcept { return *front_; }
    RenderTargetColor& back()  noexcept { return *back_; }

    void swap_buffers() {
        std::swap(front_, back_);
    }

    Size2I size() const noexcept {
        return buf1_.size();
    }

    void reset_size(Size2I new_size) {
        buf1_.reset_size(new_size);
        buf2_.reset_size(new_size);
    }

    // Implements the Bind-Draw-Unbind-Swap chain.
    void draw_and_swap(auto&& draw_function) {
        using namespace gl;
        back().framebuffer()
            .bind_draw()
            .and_then(draw_function)
            .unbind();
        swap_buffers();
    }

    PostprocessDoubleBuffer(PostprocessDoubleBuffer&& other) noexcept
        : buf1_{ std::move(other.buf1_) }
        , buf2_{ std::move(other.buf2_) }
    {
        const bool buf1_is_front = other.front_ == &other.buf1_;
        if (buf1_is_front) {
            front_ = &buf1_;
            back_  = &buf2_;
        } else /* buf2_is_front */ {
            front_ = &buf2_;
            back_  = &buf1_;
        }
    }

    PostprocessDoubleBuffer& operator=(PostprocessDoubleBuffer&& other) noexcept {
        buf1_ = std::move(other.buf1_);
        buf2_ = std::move(other.buf2_);
        const bool buf1_is_front = other.front_ == &other.buf1_;
        if (buf1_is_front) {
            front_ = &buf1_;
            back_  = &buf2_;
        } else /* buf2_is_front */ {
            front_ = &buf2_;
            back_  = &buf1_;
        }
        return *this;
    }

    PostprocessDoubleBuffer(const PostprocessDoubleBuffer&) = delete;
    PostprocessDoubleBuffer& operator=(const PostprocessDoubleBuffer& other) = delete;

};

} // namespace josh
