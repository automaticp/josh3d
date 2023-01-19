#pragma once
#include "All.hpp"
#include "TextureRenderTarget.hpp"
#include <utility>
#include <glbinding/gl/types.h>


namespace learn {


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

    TextureRenderTarget primary_target{ width, height };
    PostprocessDoubleBuffer pdb{ width, height };

    ...

    // Render the scene to some kind of primary buffer

    primary_target.framebuffer()
        .bind_as(GL_DRAW_FRAMEBUFFER)
        .and_then([&] {

            // Draw the scene here
            draw_scene_objects();

        })
        .unbind();

    // Then blit to the backbuffer of the PDB
    // with the Bind-Draw-Unbind-Swap idiom.

    pdb.back().framebuffer()
        .bind_as(GL_DRAW_FRAMEBUFFER)
        .and_then([&] {
            primary_target
                .bind_as(GL_READ_FRAMEBUFFER)
                .blit(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST)
                .unbind();
        })
        .unbind();

    pdb.swap_buffers()

    // Do the double-buffered postprocessing using PDB
    // with the Bind-Draw-Unbind-Swap loop

    for (auto&& pp : std::span(pp_stages.begin(), pp_stages.end() - 1)) {
        pdb.back().framebuffer()
            .bind_as(GL_DRAW_FRAMEBUFFER)
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
    TextureRenderTarget buf1_;
    TextureRenderTarget buf2_;

    TextureRenderTarget* front_{ &buf1_ };
    TextureRenderTarget* back_{ &buf2_ };

public:
    PostprocessDoubleBuffer(gl::GLsizei width, gl::GLsizei height)
        : buf1_{ width, height }
        , buf2_{ width, height }
    {}

    TextureHandle& front_target() noexcept { return front_->target_texture(); }
    const TextureHandle& front_target() const noexcept { return front_->target_texture(); }


    TextureRenderTarget& front() noexcept { return *front_; }
    TextureRenderTarget& back() noexcept { return *back_; }

    void swap_buffers() {
        std::swap(front_, back_);
    }

    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        buf1_.reset_size(width, height);
        buf2_.reset_size(width, height);
    }


};

} // namespace learn
