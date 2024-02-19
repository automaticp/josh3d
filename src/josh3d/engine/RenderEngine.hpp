#pragma once
#include "DefaultResources.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "PerspectiveCamera.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
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

    const Size2I&     window_size_;
    const FrameTimer& frame_timer_;


    using MainTarget = RenderTarget<
        ViewAttachment<RawTexture2D>,  // Depth
        UniqueAttachment<RawTexture2D> // Color
    >;

    UniqueAttachment<RawTexture2D> depth_{
        window_size_, { gl::GL_DEPTH24_STENCIL8 }
    };

    MainTarget make_main_target() {
        using enum GLenum;
        MainTarget tgt{
            { depth_ },
            { window_size_, { GL_RGBA16F }}
        };

        tgt.depth_attachment().texture().bind()
            .set_min_mag_filters(GL_NEAREST, GL_NEAREST)
            .set_wrap_st(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        tgt.color_attachment().texture().bind()
            .set_min_mag_filters(GL_LINEAR, GL_LINEAR)
            .set_wrap_st(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        return tgt;
    }

    SwapChain<MainTarget> main_swapchain_{
        make_main_target(), make_main_target()
    };

    inline static const RawFramebuffer<GLMutable> default_fbo_{ 0 };


public:
    // Enables RGB -> sRGB conversion at the end of the postprocessing pass.
    bool enable_srgb_conversion{ true };

    RenderEngine(
        entt::registry& registry,
        PerspectiveCamera& cam,
        const Size2I& window_size,
        const FrameTimer& frame_timer
    )
        : registry_{ registry }
        , cam_{ cam }
        , window_size_{ window_size }
        , frame_timer_{ frame_timer }
    {
        using enum GLenum;
        depth_.texture().bind()
            .set_min_mag_filters(GL_NEAREST, GL_NEAREST)
            .set_wrap_st(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
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


    RawTexture2D<GLMutable> main_depth()       noexcept { return depth_.texture(); }
    RawTexture2D<GLConst>   main_depth() const noexcept { return depth_.texture(); }

    auto& camera() noexcept { return cam_; }
    const auto& camera() const noexcept { return cam_; }

    const Size2I&     window_size() const noexcept { return window_size_; }
    const FrameTimer& frame_timer() const noexcept { return frame_timer_; }

    void reset_size(Size2I new_size) {
        main_swapchain_.resize_all(new_size);
    }

    void reset_size_from_window_size() {
        reset_size(window_size_);
    }

private:
    void execute_precompute_stages();
    void render_primary_stages();
    void render_postprocess_stages();
    void render_overlay_stages();

    template<typename StagesContainerT, typename REInterfaceT>
    void execute_stages(StagesContainerT&& stages, REInterfaceT&& engine_interface);

};








class RenderEngineCommonInterface {
protected:
    RenderEngine& engine_;

public:
    RenderEngineCommonInterface(RenderEngine& engine) : engine_{ engine } {}

    const PerspectiveCamera& camera() const noexcept { return engine_.cam_; }
    const Size2I& window_size() const noexcept { return engine_.window_size_; }
    const FrameTimer& frame_timer() const noexcept { return engine_.frame_timer_; }
};








class RenderEnginePrecomputeInterface : public RenderEngineCommonInterface {
private:
    friend class RenderEngine;
    RenderEnginePrecomputeInterface(RenderEngine& engine)
        : RenderEngineCommonInterface(engine)
    {}
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
    template<typename CallableT, typename ...Args>
    void draw(CallableT&& draw_func, Args&&... args) const {

        engine_.main_swapchain_.back_target().bind_draw()
            .and_then([&] {
                std::invoke(std::forward<CallableT>(draw_func), std::forward<Args>(args)...);
            })
            .unbind();

    }

    // The RenderEngine's main framebuffer is not exposed here because
    // there's a high chance you'll be reading from it, as you're writing
    // to it. Hear me? You! Yes, you. Think twice before implementing it again.
    //
    // I didn't allow it initially for a reason.
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
    void draw() const {
        engine_.main_swapchain_.draw_and_swap([] {
            globals::quad_primitive_mesh().draw();
        });
    }


    // Emit the draw call on the screen quad and draw directly to the front buffer.
    // DOES NOT advance the chain. You CANNOT SAMPLE THE SCREEN COLOR during this draw.
    //
    // Used as an optimization for draws that either override or blend with the screen.
    void draw_to_front() const {
        engine_.main_swapchain_.front_target().bind_draw().and_then([] {
            globals::quad_primitive_mesh().draw();
        });
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
    void draw_fullscreen_quad() const {
        engine_.default_fbo_.bind_draw().and_then([] {
            globals::quad_primitive_mesh().draw();
        });
    }

    // Effectively binds the default framebuffer as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<typename CallableT, typename ...Args>
    void draw(CallableT&& draw_func, Args&&... args) const {

        engine_.default_fbo_.bind_draw()
            .and_then([&] {
                std::invoke(std::forward<CallableT>(draw_func), std::forward<Args>(args)...);
            })
            .unbind();

    }

};



} // namespace josh
