#include "RenderEngine.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "RenderStage.hpp"
#include <chrono>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>




using namespace gl;

namespace josh {


void RenderEngine::render() {

    // Update camera.
    auto params = cam_.get_params();
    params.aspect_ratio = window_size_.aspect_ratio();
    cam_.update_params(params);


    // Precompute.
    execute_precompute_stages();


    // Primary.
    main_swapchain_.back_target().bind_draw()
        .and_then([] { glClear(GL_DEPTH_BUFFER_BIT); });

    glEnable(GL_DEPTH_TEST);
    render_primary_stages(); // To swapchain backbuffer.
    glDisable(GL_DEPTH_TEST);


    // Postprocess.
    main_swapchain_.swap_buffers();
    render_postprocess_stages(); // To swapchain (swap each draw).


    // Blit front to default (opt. sRGB)
    if (enable_srgb_conversion) { glEnable(GL_FRAMEBUFFER_SRGB); }

    main_swapchain_.front_target().bind_read()
        .and_then([this](BoundReadFramebuffer<GLMutable>& front) {
            using enum GLenum;
            default_fbo_.bind_draw()
                .blit_from(front, window_size_, window_size_,
                    GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        })
        .unbind();

    if (enable_srgb_conversion) { glDisable(GL_FRAMEBUFFER_SRGB); }

    // There are free frames on the table if you can eliminate
    // this blit by redirecting last postprocessing draw to the
    // default framebuffer. The problem is deciding which draw
    // is "last".
    //
    // We can ask each stage to tell us which draw is last, and
    // complain about perf if it doesn't comply. We run into a problem,
    // however, if no draw is made in the last stage at all
    // and are forced to blit anyway.
    //
    // The harder approach is to require each stage to be able
    // to tell us whether it will be drawing anything at all
    // before the frame even starts (starting from primary stages),
    // and then expect it to hold true until the end.
    // This is a difficult requirement because stages can technically
    // communicate through SharedStorage and the like, but
    // might be reasonable just as the assumption about stable registry.


    // Overlay.
    render_overlay_stages(); // To default framebuffer

    // Present.

}




template<typename StagesContainerT, typename REInterfaceT>
void RenderEngine::execute_stages(
    StagesContainerT&& stages,
    REInterfaceT&& engine_interface)
{
    for (auto& stage : std::forward<StagesContainerT>(stages)) {
        stage.resolve_available_time_queries();
        UniqueTimerQuery tquery;
        tquery.begin_query();

        auto t0 = std::chrono::steady_clock::now();

        stage.get()(std::forward<REInterfaceT>(engine_interface), registry_);

        auto t1 = std::chrono::steady_clock::now();
        stage.cpu_timer_.update(std::chrono::duration<float>(t1 - t0).count(), frame_timer_.delta<float>());

        tquery.end_query();
        stage.emplace_new_time_query(std::move(tquery), frame_timer_.delta<float>());
    }
}




void RenderEngine::execute_precompute_stages() {
    execute_stages(precompute_, RenderEnginePrecomputeInterface{ *this });
}

void RenderEngine::render_primary_stages() {
    execute_stages(primary_, RenderEnginePrimaryInterface{ *this });
}

void RenderEngine::render_postprocess_stages() {
    execute_stages(postprocess_, RenderEnginePostprocessInterface{ *this });
}

void RenderEngine::render_overlay_stages() {
    execute_stages(overlay_, RenderEngineOverlayInterface{ *this });
}




} // namespace josh
