#include "RenderEngine.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <cassert>


using namespace gl;

namespace learn {




void RenderEngine::render() {
    using namespace gl;

    main_target_.framebuffer()
        .bind_as(GL_DRAW_FRAMEBUFFER)
        .and_then([] { glClear(GL_DEPTH_BUFFER_BIT); });

    glEnable(GL_DEPTH_TEST);
    render_primary_stages();

    // TODO: Remove the blit by drawing to the desired target right away.

    if (pp_stages_.empty()) {

        main_target_.framebuffer()
            .bind_as(GL_READ_FRAMEBUFFER)
            .and_then_with_self([this](BoundFramebuffer& fbo) {

                BoundFramebuffer::unbind_as(GL_DRAW_FRAMEBUFFER);
                fbo.blit(0, 0, main_target_.width(), main_target_.height(),
                    0, 0, window_size().width, window_size().height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);

            })
            .unbind();

    } else /* has postprocessing */ {

        ppdb_.draw_and_swap([this] {
            main_target_.framebuffer()
                .bind_as(GL_READ_FRAMEBUFFER)
                .and_then_with_self([this](BoundFramebuffer& fbo) {

                    fbo.blit(0, 0, main_target_.width(), main_target_.height(),
                        0, 0, ppdb_.back().width(), ppdb_.back().height(),
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);

                })
                .unbind();
        });

        glDisable(GL_DEPTH_TEST);
        render_postprocess_stages();
    }
}




void RenderEngine::render_primary_stages() {
    for (current_stage_ = 0; current_stage_ < stages_.size(); ++current_stage_) {
        stages_[current_stage_](PrimaryInterface{ *this }, registry_);
    }
}


void RenderEngine::render_postprocess_stages() {
    for (current_pp_stage_ = 0; current_pp_stage_ < pp_stages_.size(); ++current_pp_stage_) {
        pp_stages_[current_pp_stage_](PostprocessInterface{ *this }, registry_);
    }
}





} // namespace learn
