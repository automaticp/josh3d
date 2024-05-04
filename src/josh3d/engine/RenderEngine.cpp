#include "RenderEngine.hpp"
#include "GLFramebuffer.hpp"
#include "GLObjects.hpp"
#include "RenderStage.hpp"
#include "WindowSizeCache.hpp"
#include <chrono>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <utility>




namespace josh {


void RenderEngine::render() {

    // Update camera.
    auto params = cam_.get_params();
    params.aspect_ratio = main_resolution().aspect_ratio();
    cam_.update_params(params);
    update_camera_ubo();

    // Update viewport.
    glapi::set_viewport({ {}, main_resolution() });


    // Precompute.
    execute_precompute_stages();

    // Primary.
    {
        auto bound_fbo = main_swapchain_.back_target().bind_draw();
        glapi::clear_depth_stencil_buffer(bound_fbo, 1.0f, 0);
        bound_fbo.unbind();
    }

    glapi::enable(Capability::DepthTesting);
    render_primary_stages(); // To swapchain backbuffer.
    glapi::disable(Capability::DepthTesting);

    // Postprocess.
    main_swapchain_.swap_buffers();
    render_postprocess_stages(); // To swapchain (swap each draw).


    // Blit front to default (opt. sRGB)
    if (enable_srgb_conversion) { glapi::enable(Capability::SRGBConversion); }

    main_swapchain_.front_target().framebuffer().blit_to(
        default_fbo_,
        { {}, main_resolution()           }, // Internal rendering resolution.
        { {}, globals::window_size.size() }, // This is technically window size and can technically differ, technically.
        BufferMask::ColorBit | BufferMask::DepthBit,
        BlitFilter::Nearest
    );

    if (enable_srgb_conversion) { glapi::disable(Capability::SRGBConversion); }

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




void RenderEngine::update_camera_ubo() noexcept {
    const auto&     params       = cam_.get_params();
    const glm::mat4 view         = cam_.view_mat();
    const glm::mat4 proj         = cam_.projection_mat();
    const glm::mat4 projview     = proj * view;
    const glm::mat4 inv_view     = glm::inverse(view);
    const glm::mat3 normal_view  = glm::transpose(inv_view);
    const glm::mat4 inv_proj     = glm::inverse(proj);
    const glm::mat4 inv_projview = glm::inverse(projview);

    const CameraData data{
        .position_ws  = cam_.transform.position(),
        .z_near       = params.z_near,
        .z_far        = params.z_far,
        .view         = view,
        .proj         = proj,
        .projview     = projview,
        .inv_view     = inv_view,
        .normal_view  = normal_view,
        .inv_proj     = inv_proj,
        .inv_projview = inv_projview
    };

    camera_ubo_->upload_data({ &data, 1 });
}




template<typename StagesContainerT, typename REInterfaceT>
void RenderEngine::execute_stages(
    StagesContainerT& stages,
    REInterfaceT&     engine_interface)
{
    if (capture_stage_timings) {
        for (auto& stage : std::forward<StagesContainerT>(stages)) {
            stage.gpu_timer_.resolve_available_time_queries();

            stage.cpu_timer_.averaging_interval = stage_timing_averaging_interval_s;
            stage.gpu_timer_.set_averaging_interval(stage_timing_averaging_interval_s);

            UniqueQueryTimeElapsed tquery;
            tquery->begin_query();

            auto t0 = std::chrono::steady_clock::now();


            stage.get()(engine_interface);


            auto t1 = std::chrono::steady_clock::now();
            stage.cpu_timer_.update(std::chrono::duration<float>(t1 - t0).count(), frame_timer_.delta<float>());

            tquery->end_query();
            stage.gpu_timer_.emplace_new_time_query(std::move(tquery), frame_timer_.delta<float>());
        }
    } else {
        for (auto& stage : std::forward<StagesContainerT>(stages)) {
            stage.get()(engine_interface);
        }
    }
}




void RenderEngine::execute_precompute_stages() {
    RenderEnginePrecomputeInterface proxy{ *this };
    execute_stages(precompute_, proxy);
}

void RenderEngine::render_primary_stages() {
    RenderEnginePrimaryInterface proxy{ *this };
    execute_stages(primary_, proxy);
}

void RenderEngine::render_postprocess_stages() {
    RenderEnginePostprocessInterface proxy{ *this };
    execute_stages(postprocess_, proxy);
}

void RenderEngine::render_overlay_stages() {
    RenderEngineOverlayInterface proxy{ *this };
    execute_stages(overlay_, proxy);
}




} // namespace josh
