#pragma once
#include "Attachments.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "GLTextures.hpp"
#include "GPULayout.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "MeshRegistry.hpp"
#include "Primitives.hpp"
#include "RenderTarget.hpp"
#include "Region.hpp"
#include "RenderStage.hpp"
#include "SwapChain.hpp"
#include <entt/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <ranges>
#include <string>
#include <utility>
#include <vector>




namespace josh {


/*
Implementation base for wrapper types that constrain the actions avaliable
to be done with the engine during various stages.
*/
class RenderEngineCommonInterface;


/*
A wrapper object that constrains the set of actions avaliable
during precompute stages. Passed to the precompute stages
as a proxy for RenderEngine.
*/
class RenderEnginePrecomputeInterface;


/*
A wrapper object that constrains the set of actions avaliable
during primary stages. Passed to the primary stages
as a proxy for RenderEngine.
*/
class RenderEnginePrimaryInterface;


/*
A wrapper object that constrains the set of actions avaliable
during postprocessing stages. Passed to the postprocessing stages
as a proxy for RenderEngine.
*/
class RenderEnginePostprocessInterface;


/*
A wrapper object that constrains the set of actions avaliable
during overlay stages. Passed to the overlay stages
as a proxy for RenderEngine.
*/
class RenderEngineOverlayInterface;








class RenderEngine {
public:
    enum class HDRFormat : GLuint {
        R11F_G11F_B10F  = GLuint(InternalFormat::R11F_G11F_B10F),
        RGB16F          = GLuint(InternalFormat::RGB16F),
        RGB32F          = GLuint(InternalFormat::RGB32F), // Don't know why you'd want this but...
    };

    HDRFormat main_buffer_format;
    Size2I    main_resolution;

    // Enables RGB -> sRGB conversion at the end of the postprocessing pass.
    bool  enable_srgb_conversion{ true };
    // Enables profiling of GPU/CPU times taken per each stage.
    bool  capture_stage_timings { true };
    // The wall-time interval between updates of the timers, in seconds.
    // Note that the GPU timing is asyncronous and might lag behind by a frame or two.
    float stage_timing_averaging_interval_s{ 0.5f };


    RenderEngine(
        entt::registry&     registry,
        const MeshRegistry& mesh_registry,
        const Primitives&   primitives,
        const Size2I&       main_resolution,
        HDRFormat           main_buffer_format,
        const FrameTimer&   frame_timer); // TODO: Should not be a reference. TODO: Why? I forgot...


    void render();


    auto precompute_stages_view()  noexcept { return std::views::all(precompute_);  }
    auto primary_stages_view()     noexcept { return std::views::all(primary_);     }
    auto postprocess_stages_view() noexcept { return std::views::all(postprocess_); }
    auto overlay_stages_view()     noexcept { return std::views::all(overlay_);   }


    template<precompute_render_stage StageT>
    void add_next_precompute_stage(std::string name, StageT&& stage) {
        precompute_.emplace_back(
            PrecomputeStage{
                std::move(name), AnyPrecomputeStage{ std::forward<StageT>(stage) }
            }
        );
    }

    template<primary_render_stage StageT>
    void add_next_primary_stage(std::string name, StageT&& stage) {
        primary_.emplace_back(
            PrimaryStage{
                std::move(name), AnyPrimaryStage{ std::forward<StageT>(stage) }
            }
        );
    }

    template<postprocess_render_stage StageT>
    void add_next_postprocess_stage(std::string name, StageT&& stage) {
        postprocess_.emplace_back(
            PostprocessStage{
                std::move(name), AnyPostprocessStage{ std::forward<StageT>(stage) }
            }
        );
    }

    template<overlay_render_stage StageT>
    void add_next_overlay_stage(std::string name, StageT&& stage) {
        overlay_.emplace_back(
            OverlayStage{
                std::move(name), AnyOverlayStage{ std::forward<StageT>(stage) }
            }
        );
    }


    RawTexture2D<GLConst>   main_depth_texture()          const noexcept { return main_depth_.texture(); }
    const auto&             main_depth_attachment()       const noexcept { return main_depth_;           }
    auto                    share_main_depth_attachment()       noexcept { return main_depth_.share();   }

    RawTexture2D<GLConst>   main_color_texture()          const noexcept { return main_swapchain_.back_target().color_attachment().texture(); }
    const auto&             main_color_attachment()       const noexcept { return main_swapchain_.back_target().color_attachment();           }
    auto                    share_main_color_attachment()       noexcept { return main_swapchain_.back_target().share_color_attachment();     }

    const Primitives& primitives() const noexcept { return primitives_; }
    const FrameTimer& frame_timer() const noexcept { return frame_timer_; }


private:
    friend RenderEngineCommonInterface;
    friend RenderEnginePrecomputeInterface;
    friend RenderEnginePrimaryInterface;
    friend RenderEnginePostprocessInterface;
    friend RenderEngineOverlayInterface;


    std::vector<PrecomputeStage>  precompute_;
    std::vector<PrimaryStage>     primary_;
    std::vector<PostprocessStage> postprocess_;
    std::vector<OverlayStage>     overlay_;

    entt::registry&     registry_;
    const MeshRegistry& mesh_registry_;
    const Primitives&   primitives_;
    const FrameTimer&   frame_timer_;


    using MainTarget = RenderTarget<
        SharedAttachment<Renderable::Texture2D>,    // Depth
        ShareableAttachment<Renderable::Texture2D>  // Color
    >;
    using DepthAttachment = ShareableAttachment<Renderable::Texture2D>;

    static auto make_main_depth(const Size2I& resolution)
        -> DepthAttachment;

    DepthAttachment main_depth_;

    static auto make_main_swapchain(
        const Size2I&    resolution,
        InternalFormat   iformat,
        DepthAttachment& depth)
            -> SwapChain<MainTarget>;

    SwapChain<MainTarget> main_swapchain_;


    inline static const RawDefaultFramebuffer<GLMutable> default_fbo_;


    struct CameraDataGPU {
        alignas(std430::align_vec3)  glm::vec3   position_ws; // World-space position
        alignas(std430::align_float) float       z_near;
        alignas(std430::align_float) float       z_far;
        alignas(std430::align_vec4)  glm::mat4   view;
        alignas(std430::align_vec4)  glm::mat4   proj;
        alignas(std430::align_vec4)  glm::mat4   projview;
        alignas(std430::align_vec4)  glm::mat4   inv_view;
        alignas(std430::align_vec4)  glm::mat3x4 normal_view; // mat3, but padding is needed for each column in std140
        alignas(std430::align_vec4)  glm::mat4   inv_proj;
        alignas(std430::align_vec4)  glm::mat4   inv_projview;
    };

    CameraDataGPU               camera_data_;
    UniqueBuffer<CameraDataGPU> camera_ubo_{ allocate_buffer<CameraDataGPU>(NumElems{ 1 }) };
    void update_camera_data(
        const glm::mat4& view,
        const glm::mat4& proj,
        float            z_near,
        float            z_far) noexcept;


    void execute_precompute_stages();
    void render_primary_stages();
    void render_postprocess_stages();
    void render_overlay_stages();

    template<typename StagesContainerT, typename REInterfaceT>
    void execute_stages(StagesContainerT& stages, REInterfaceT& engine_interface);

};








class RenderEngineCommonInterface {
public:
    auto meshes()       const noexcept -> const MeshRegistry&   { return engine_.mesh_registry_; }
    auto registry()     const noexcept -> const entt::registry& { return engine_.registry_;      }
    auto primitives()   const noexcept -> const Primitives&     { return engine_.primitives_;    }
    auto camera_data()  const noexcept -> const auto&           { return engine_.camera_data_;   }
    // TODO: Something about window resolution being separate?
    // TODO: Also main_resolution() is not accurate in Overlay stages.
    auto main_resolution() const noexcept -> const Size2I&     { return engine_.main_resolution; }
    auto frame_timer()     const noexcept -> const FrameTimer& { return engine_.frame_timer_;    }

    auto bind_camera_ubo(GLuint index = 0) const noexcept
        -> BindToken<BindingIndexed::UniformBuffer>
    {
        return engine_.camera_ubo_->bind_to_index<BufferTargetIndexed::Uniform>(index);
    }

protected:
    RenderEngine& engine_;
    RenderEngineCommonInterface(RenderEngine& engine) : engine_{ engine } {}
};








class RenderEnginePrecomputeInterface : public RenderEngineCommonInterface {
private:
    friend class RenderEngine;
    RenderEnginePrecomputeInterface(RenderEngine& engine)
        : RenderEngineCommonInterface(engine)
    {}

public:
    auto registry()       noexcept ->       entt::registry& { return engine_.registry_; }
    auto registry() const noexcept -> const entt::registry& { return engine_.registry_; }
};








class RenderEnginePrimaryInterface : public RenderEngineCommonInterface {
private:
    friend class RenderEngine;
    RenderEnginePrimaryInterface(RenderEngine& engine)
        : RenderEngineCommonInterface(engine)
    {}

public:
    // Effectively binds the main render target as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> CallableT>
    void draw(CallableT&& draw_func) {
        BindGuard bound_fbo = engine_.main_swapchain_.back_target().bind_draw();
        draw_func(bound_fbo.token());
    }

    // The RenderEngine's main framebuffer is not exposed here because
    // there's a high chance you'll be reading from it, as you're writing
    // to it. Hear me? You! Yes, you. Think twice before implementing it again.
    //
    // I didn't allow it initially for a reason.

    const auto& main_depth_attachment()       const noexcept { return engine_.main_depth_attachment();       }
    auto        share_main_depth_attachment()       noexcept { return engine_.share_main_depth_attachment(); }

    const auto& main_color_attachment()       const noexcept { return engine_.main_color_attachment();       }
    auto        share_main_color_attachment()       noexcept { return engine_.share_main_color_attachment(); }

};








class RenderEnginePostprocessInterface : public RenderEngineCommonInterface {
private:
    friend class RenderEngine;
    RenderEnginePostprocessInterface(RenderEngine& engine)
        : RenderEngineCommonInterface(engine)
    {}

public:
    RawTexture2D<GLConst> screen_color() const noexcept {
        return engine_.main_swapchain_.front_target().color_attachment().texture();
    }

    RawTexture2D<GLConst> screen_depth() const noexcept {
        return engine_.main_swapchain_.front_target().depth_attachment().texture();
        // return engine_.depth_; // Is the same.
    }

    // Emit the draw call on the screen quad and adjust the render target state
    // for the next stage in the chain.
    //
    // The screen color texture is INVALIDATED for sampling after this call.
    // You have to call screen_color() again and bind the returned texture
    // in order to sample the screen in the next call to draw().
    void draw(BindToken<Binding::Program> bound_program) {
        engine_.main_swapchain_.draw_and_swap([&](BindToken<Binding::DrawFramebuffer> bound_fbo) {
            engine_.primitives().quad_mesh().draw(bound_program, bound_fbo);
        });
    }


    // Emit the draw call on the screen quad and draw directly to the front buffer.
    // DOES NOT advance the chain. You CANNOT SAMPLE THE SCREEN COLOR during this draw.
    //
    // Used as an optimization for draws that either override or blend with the screen.
    void draw_to_front(BindToken<Binding::Program> bound_program) {
        BindGuard bound_fbo{ engine_.main_swapchain_.front_target().bind_draw() };
        engine_.primitives().quad_mesh().draw(bound_program, bound_fbo);
    }

};






class RenderEngineOverlayInterface : public RenderEngineCommonInterface {
private:
    friend class RenderEngine;
    RenderEngineOverlayInterface(RenderEngine& engine)
        : RenderEngineCommonInterface(engine)
    {}

public:
    // Emit the draw call on the screen quad and draw directly to the default buffer.
    void draw_fullscreen_quad(BindToken<Binding::Program> bound_program) {
        BindGuard bound_fbo{ engine_.default_fbo_.bind_draw() };
        engine_.primitives().quad_mesh().draw(bound_program, bound_fbo);
    }

    // Effectively binds the default framebuffer as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> CallableT>
    void draw(CallableT&& draw_func) {
        draw_func(engine_.default_fbo_.bind_draw());
    }

};



} // namespace josh
