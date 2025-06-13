#include "RenderEngine.hpp"
#include "Active.hpp"
#include "EnumUtils.hpp"
#include "Camera.hpp"
#include "GLFramebuffer.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "MeshRegistry.hpp"
#include "RenderStage.hpp"
#include "Skeleton.hpp"
#include "Transform.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <chrono>


namespace josh {

RenderEngine::RenderEngine(
    Extent2I  main_resolution,
    HDRFormat main_format
)
    : main_buffer_format{ main_format     }
    , main_resolution   { main_resolution }
    , main_depth_       {
        make_main_depth(main_resolution)
    }
    , main_swapchain_   {
        make_main_swapchain(
            main_resolution,
            enum_cast<InternalFormat>(main_buffer_format),
            main_depth_
        )
    }
{}

void RenderEngine::render(
    Registry&           registry,
    const MeshRegistry& mesh_registry,
    const Primitives&   primitives,
    const Extent2I&     window_resolution,
    const FrameTimer&   frame_timer)
{
    // Update main render buffer if necessary.
    const bool resolution_changed =
        main_resolution != main_swapchain_.resolution();

    const bool iformat_changed =
        enum_cast<InternalFormat>(main_buffer_format) !=
            main_swapchain_.back_target().color_attachment().internal_format();

    if (resolution_changed or iformat_changed)
    {
        main_depth_.resize(main_resolution);

        if (iformat_changed)
        {
            const InternalFormat new_iformat = enum_cast<InternalFormat>(main_buffer_format);
            // Will also reattach new depth.
            main_swapchain_ = make_main_swapchain(main_resolution, new_iformat, main_depth_);
        }
        else
        {
            main_swapchain_.resize(main_resolution);
            main_swapchain_.front_target().reset_depth_attachment(main_depth_.share());
            main_swapchain_.back_target() .reset_depth_attachment(main_depth_.share());
        }
    }

    // Update camera.
    // TODO: Orthographic has no notion of aspect_ratio.
    // TODO: Should this be done after precompute? As precompute can change what's active.
    // TODO: Absence if an active camera, in general, is pretty bad. Do we even render?
    //
    // FIXME: The camera should likely be passed to render() directly. Let the user figure
    // out which camera to use. We'll just build the matrices and a UBO from it.
    if (const Handle handle = get_active<Camera>(registry))
    {
        Camera& camera = handle.get<Camera>();
        auto params = camera.get_params();
        params.aspect_ratio = main_resolution.aspect_ratio();
        camera.update_params(params);

        mat4 view = glm::identity<mat4>();
        if (auto* mtf = handle.try_get<MTransform>())
            view = inverse(mtf->model()); // model is W2C, view is C2W.

        update_camera_data(view, camera.projection_mat(), params.z_near, params.z_far);
    }

    // Update viewport.
    glapi::set_viewport({ {}, main_resolution });

    auto execute_stages = [&](
        auto& stages,
        auto  interface)
    {
        if (capture_stage_timings)
        {
            for (auto& stage : stages)
            {
                stage.gpu_timer_.resolve_available_time_queries();

                stage.cpu_timer_.averaging_interval = stage_timing_averaging_interval_s;
                stage.gpu_timer_.set_averaging_interval(stage_timing_averaging_interval_s);

                UniqueQueryTimeElapsed tquery;
                tquery->begin_query();

                auto t0 = std::chrono::steady_clock::now();

                stage.get()(interface);

                auto t1 = std::chrono::steady_clock::now();
                stage.cpu_timer_.update(std::chrono::duration<float>(t1 - t0).count(), frame_timer.delta<float>());

                tquery->end_query();
                stage.gpu_timer_.emplace_new_time_query(MOVE(tquery), frame_timer.delta<float>());
            }
        }
        else
        {
            for (auto& stage : stages)
                stage.get()(interface);
        }
    };

    const RenderEngineCommonInterface interface_common = {
        ._engine        = *this,
        ._registry      = registry,
        ._mesh_registry = mesh_registry,
        ._primitives    = primitives,
        ._frame_timer   = frame_timer
    };
    const RenderEnginePrecomputeInterface  interface_precompute  = { interface_common };
    const RenderEnginePrimaryInterface     interface_primary     = { interface_common };
    const RenderEnginePostprocessInterface interface_postprocess = { interface_common };
    const RenderEngineOverlayInterface     interface_overlay     = { interface_common };

    // Precompute.
    execute_stages(precompute_, interface_precompute);

    // Primary.
    {
        BindGuard bound_fbo = main_swapchain_.back_target().bind_draw();
        glapi::clear_depth_stencil_buffer(bound_fbo, 1.0f, 0);
    }

    // To swapchain backbuffer.
    glapi::enable(Capability::DepthTesting);
    execute_stages(primary_, interface_primary);
    glapi::disable(Capability::DepthTesting);

    // Postprocess.
    main_swapchain_.swap_buffers();
    // To swapchain (swap each draw).
    execute_stages(postprocess_, interface_postprocess);

    // Blit front to default (opt. sRGB)
    if (enable_srgb_conversion) glapi::enable(Capability::SRGBConversion);

    main_swapchain_.front_target().framebuffer().blit_to(
        default_fbo_,
        { {}, main_resolution   }, // Internal rendering resolution.
        { {}, window_resolution }, // This is technically window size and can technically differ, technically.
        BufferMask::ColorBit | BufferMask::DepthBit,
        BlitFilter::Nearest
    );

    if (enable_srgb_conversion) glapi::disable(Capability::SRGBConversion);

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
    execute_stages(overlay_, interface_overlay);

    // Present.
}

auto RenderEngine::make_main_depth(const Extent2I& resolution)
    -> DepthAttachment
{
    DepthAttachment depth{ InternalFormat::Depth24_Stencil8 };
    depth.resize(resolution);
    return depth;
}

auto RenderEngine::make_main_swapchain(
    const Extent2I&  resolution,
    InternalFormat   iformat,
    DepthAttachment& depth)
        -> SwapChain<MainTarget>
{
    return {
        MainTarget{ resolution, depth.share(), { iformat } },
        MainTarget{ resolution, depth.share(), { iformat } },
    };
}

void RenderEngine::update_camera_data(
    const mat4& view,
    const mat4& proj,
    float       z_near,
    float       z_far) noexcept
{
    const mat4 projview     = proj * view;
    const mat4 inv_view     = inverse(view);
    const mat3 normal_view  = transpose(inv_view);
    const mat4 inv_proj     = inverse(proj);
    const mat4 inv_projview = inverse(projview);
    const vec3 position_ws  = inv_view[3];

    camera_data_ = CameraDataGPU{
        .position_ws  = position_ws,
        .z_near       = z_near,
        .z_far        = z_far,
        .view         = view,
        .proj         = proj,
        .projview     = projview,
        .inv_view     = inv_view,
        .normal_view  = normal_view,
        .inv_proj     = inv_proj,
        .inv_projview = inv_projview
    };

    camera_ubo_->upload_data({ &camera_data_, 1 });
}


} // namespace josh
