#include "RenderEngine.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <cassert>


using namespace gl;

namespace josh {




/*
Just some thoughts:

The main render target of the engine is a SwapChain
with shared Depth.

The primary passes all draw to the backbuffer of the
SwapChain. Then it swaps and the postprocessing begins.

At the end of postprocessing, the result is blitted into
the default framebuffer.
(sRGB conversion can be done here).

Additionally, you can have a bunch of overlays drawn
after this, which can sample from the pre-blit SwapChain buffer.

Depending on the choice of gamma-correction, if it was
applied manually, you'll be sampling sRGB colors.
If it was applied as an sRGB blit, then you'll be sampling
linear colors, since that's what is left in the SwapChain.

What a mess.

There probably has to be pre-sRGB and post-sRGB postprocessing.
This has the same implications as Overlay stages, I think.
*/



void RenderEngine::render() {

    // Update camera.
    auto params = cam_.get_params();
    params.aspect_ratio = window_size_.aspect_ratio();
    cam_.update_params(params);


    main_swapchain_.back_target().bind_draw()
        .and_then([] { glClear(GL_DEPTH_BUFFER_BIT); });

    glEnable(GL_DEPTH_TEST);
    render_primary_stages();


    if (pp_stages_.empty()) {

        main_swapchain_.back_target().bind_read()
            .and_then([this](BoundReadFramebuffer<GLMutable>& src_fb) {

                // FIXME: Kinda awkward because there's no default binding object.
                // Way to fix this is so far unknown.
                RawFramebuffer<GLMutable> default_fb{ 0 };

                auto [src_w, src_h] = main_swapchain_.back_target().color_attachment().size();
                auto [dst_w, dst_h] = window_size_;

                default_fb.bind_draw()
                    .blit_from(
                        src_fb,
                        0, 0, src_w, src_h,
                        0, 0, dst_w, dst_h,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST
                    )
                    .unbind();
            })
            .unbind();

    } else /* has postprocessing */ {

        // So that the next PP stage could sample from the results of the main pass.
        main_swapchain_.swap_buffers();

        glDisable(GL_DEPTH_TEST);

        render_postprocess_stages();
    }
}




void RenderEngine::render_primary_stages() {
    for (current_stage_ = 0; current_stage_ < stages_.size(); ++current_stage_) {
        stages_[current_stage_](RenderEnginePrimaryInterface{ *this }, registry_);
    }
}


void RenderEngine::render_postprocess_stages() {
    for (current_pp_stage_ = 0; current_pp_stage_ < pp_stages_.size(); ++current_pp_stage_) {
        pp_stages_[current_pp_stage_](RenderEnginePostprocessInterface{ *this }, registry_);
    }
}





} // namespace josh
