#include "RenderEngine.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <cassert>


using namespace gl;

namespace josh {




void RenderEngine::render() {
    using namespace gl;

    // Update camera.
    auto params = cam_.get_params();
    params.aspect_ratio = window_size_.aspect_ratio();
    cam_.update_params(params);


    main_target_.framebuffer()
        .bind_draw()
        .and_then([] { glClear(GL_DEPTH_BUFFER_BIT); });

    glEnable(GL_DEPTH_TEST);
    render_primary_stages();

    // TODO: Remove the blit by drawing to the desired target right away.

    if (pp_stages_.empty()) {

        main_target_.framebuffer()
            .bind_read()
            .and_then([this](BoundReadFramebuffer& fbo) {

                BoundDrawFramebuffer::unbind();

                glBlitFramebuffer(
                    0, 0, main_target_.width(), main_target_.height(),
                    0, 0, window_size().width, window_size().height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST
                );
            })
            .unbind();

    } else /* has postprocessing */ {

        ppdb_.draw_and_swap([this] {
            main_target_.framebuffer()
                .bind_read()
                .and_then([this](BoundReadFramebuffer& fbo) {

                    // FIXME: Here I can pass the BoundDrawFramebuffer through the
                    // draw_and_swap() callback. PPDB backbuffer is bound implicitly otherwise.
                    glBlitFramebuffer(
                        0, 0, main_target_.width(), main_target_.height(),
                        0, 0, ppdb_.back().width(), ppdb_.back().height(),
                        GL_COLOR_BUFFER_BIT, GL_NEAREST
                    );

                })
                .unbind();
        });

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
