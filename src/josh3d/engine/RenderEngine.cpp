#include "RenderEngine.hpp"
#include "GLFramebuffer.hpp"
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
            .and_then([this](BoundReadFramebuffer<GLMutable>&) {

                // FIXME: Kinda awkward because there's no default binding object.
                // Way to fix this is so far unknown.
                BoundDrawFramebuffer<GLMutable>::unbind();

                auto [src_w, src_h] = main_target_.size();
                auto [dst_w, dst_h] = window_size_;

                glBlitFramebuffer(
                    0, 0, src_w, src_h,
                    0, 0, dst_w, dst_h,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST
                );
            })
            .unbind();

    } else /* has postprocessing */ {

        ppdb_.draw_and_swap([this](BoundDrawFramebuffer<GLMutable>& dst_fbo) {
            main_target_.framebuffer()
                .bind_read()
                .and_then([&, this](BoundReadFramebuffer<GLMutable>& src_fbo) {

                    auto [src_w, src_h] = main_target_.size();
                    auto [dst_w, dst_h] = ppdb_.back().size();

                    src_fbo.blit_to(dst_fbo,
                        0, 0, src_w, src_h, 0, 0, dst_w, dst_h,
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
        stages_[current_stage_](RenderEnginePrimaryInterface{ *this }, registry_);
    }
}


void RenderEngine::render_postprocess_stages() {
    for (current_pp_stage_ = 0; current_pp_stage_ < pp_stages_.size(); ++current_pp_stage_) {
        pp_stages_[current_pp_stage_](RenderEnginePostprocessInterface{ *this }, registry_);
    }
}





} // namespace josh
