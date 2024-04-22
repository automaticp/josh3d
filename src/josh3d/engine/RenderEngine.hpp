#pragma once
#include "Attachments.hpp"
#include "GLAPIBinding.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "GLTextures.hpp"
#include "PerspectiveCamera.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "Primitives.hpp"
#include "RenderTarget.hpp"
#include "Size.hpp"
#include "RenderStage.hpp"
#include "SwapChain.hpp"
#include <entt/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <functional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>




// WIP



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
    // Enables RGB -> sRGB conversion at the end of the postprocessing pass.
    bool  enable_srgb_conversion{ true };
    // Enables profiling of GPU/CPU times taken per each stage.
    bool  capture_stage_timings { true };
    // The wall-time interval between updates of the timers, in seconds.
    // Note that the GPU timing is asyncronous and might lag behind by a frame or two.
    float stage_timing_averaging_interval_s{ 0.5f };


    RenderEngine(
        entt::registry&    registry,
        PerspectiveCamera& cam,        // TODO: Should not be a reference.
        const Primitives&  primitives,
        const Size2I&      resolution,
        const FrameTimer&  frame_timer // TODO: Should not be a reference.
    )
        : registry_       { registry     }
        , cam_            { cam          }
        , primitives_     { primitives   }
        , frame_timer_    { frame_timer  }
        , main_swapchain_ { make_main_target(resolution), make_main_target(resolution) }
    {
        // Because we are sharing the depth in the swapchain, we have to bother to sync the size manually.
        resize_depth(main_resolution());
    }

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


    RawTexture2D<GLConst>   main_depth_texture()          const noexcept { return depth_.texture(); }
    const auto&             main_depth_attachment()       const noexcept { return depth_;           }
    auto                    share_main_depth_attachment()       noexcept { return depth_.share();   }

    Size2I main_resolution() const noexcept { return main_swapchain_.resolution(); }

    auto&       camera()       noexcept { return cam_; }
    const auto& camera() const noexcept { return cam_; }

    const Primitives& primitives() const noexcept { return primitives_; }

    const FrameTimer& frame_timer() const noexcept { return frame_timer_; }

    void resize(const Size2I& new_resolution) {
        main_swapchain_.resize(new_resolution);
        resize_depth(new_resolution);
    }



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

    entt::registry& registry_;

    PerspectiveCamera& cam_;

    const Primitives& primitives_;

    const FrameTimer& frame_timer_;


    using MainTarget = RenderTarget<
        SharedAttachment<Renderable::Texture2D>, // Depth
        UniqueAttachment<Renderable::Texture2D>  // Color
    >;

    ShareableAttachment<Renderable::Texture2D> depth_{
        // TODO: Floating-point depth is not blittable to the default fbo.
        // How would we do it then?
        InternalFormat::Depth24_Stencil8
    };

    MainTarget make_main_target(const Size2I& resolution) {
        return {
            resolution,
            depth_.share(),
            { InternalFormat::RGBA16F }
        };
    }

    SwapChain<MainTarget> main_swapchain_;


    inline static const RawDefaultFramebuffer<GLMutable> default_fbo_;



    void resize_depth(const Size2I& new_resolution) {
        depth_.resize(new_resolution);
        if (!main_swapchain_.back_target().depth_attachment().is_shared_from(depth_)) {
            main_swapchain_.front_target().reset_depth_attachment(depth_.share());
            main_swapchain_.back_target() .reset_depth_attachment(depth_.share());
        }
    }

    void execute_precompute_stages();
    void render_primary_stages();
    void render_postprocess_stages();
    void render_overlay_stages();

    template<typename StagesContainerT, typename REInterfaceT>
    void execute_stages(StagesContainerT& stages, REInterfaceT& engine_interface);

};








class RenderEngineCommonInterface {
protected:
    RenderEngine& engine_;
    RenderEngineCommonInterface(RenderEngine& engine) : engine_{ engine } {}

public:
    const entt::registry&    registry()        const noexcept { return engine_.registry_;   }
    const PerspectiveCamera& camera()          const noexcept { return engine_.cam_;        }
    const Primitives&        primitives()      const noexcept { return engine_.primitives_; }
    // TODO: Something about window resolution being separate?
    // TODO: Also main_resolution() is not accurate in Overlay stages.
    Size2I                   main_resolution() const noexcept { return engine_.main_resolution(); }
    const FrameTimer&        frame_timer()     const noexcept { return engine_.frame_timer_;      }
};








class RenderEnginePrecomputeInterface : public RenderEngineCommonInterface {
private:
    friend class RenderEngine;
    RenderEnginePrecomputeInterface(RenderEngine& engine)
        : RenderEngineCommonInterface(engine)
    {}

public:
          entt::registry& registry()       noexcept { return engine_.registry_; }
    const entt::registry& registry() const noexcept { return engine_.registry_; }

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
        auto bind_token = engine_.main_swapchain_.back_target().bind_draw();

        std::invoke(std::forward<CallableT>(draw_func), bind_token);

        bind_token.unbind();
    }

    // The RenderEngine's main framebuffer is not exposed here because
    // there's a high chance you'll be reading from it, as you're writing
    // to it. Hear me? You! Yes, you. Think twice before implementing it again.
    //
    // I didn't allow it initially for a reason.

    const auto& main_depth_attachment()       const noexcept { return engine_.main_depth_attachment();       }
    auto        share_main_depth_attachment()       noexcept { return engine_.share_main_depth_attachment(); }

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
        auto bound_fbo = engine_.main_swapchain_.front_target().bind_draw();
        engine_.primitives().quad_mesh().draw(bound_program, bound_fbo);
        bound_fbo.unbind();
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
        auto bound_fbo = engine_.default_fbo_.bind_draw();
        engine_.primitives().quad_mesh().draw(bound_program, bound_fbo);
    }

    // Effectively binds the default framebuffer as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> CallableT>
    void draw(CallableT&& draw_func) {
        std::invoke(std::forward<CallableT>(draw_func), engine_.default_fbo_.bind_draw());
    }

};



} // namespace josh
