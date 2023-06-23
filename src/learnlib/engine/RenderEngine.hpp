#pragma once
#include "Camera.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "PostprocessDoubleBuffer.hpp"
#include "PostprocessRenderer.hpp"
#include "RenderTargetColorAndDepth.hpp"
#include "UniqueFunction.hpp"
#include "WindowSize.hpp"
#include "RenderStage.hpp"
#include <entt/fwd.hpp>
#include <glbinding/gl/gl.h>
#include <cassert>
#include <concepts>
#include <functional>
#include <utility>
#include <vector>


// WIP



namespace learn {


/*
Implementation base for wrapper types that constrain the actions avaliable
to be done with the engine during primary and postprocessing stages.
*/
class RenderEngineCommonInterface;


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

There are multiple modes of operation in terms of render targets and framebuffers:

1. No postprocessing:
    Primary draws are made to the default backbuffer.

2. One postprocessing stage:
    Primary draws are made to the backbuffer of PPDB (no swapping).
    The postprocessing draw is made directly to the default backbuffer.

    FIXME: this is actually wrong because PPDB has no depth buffer
    that can be sampled later. The color can be overwritten there,
    but we need the scene depth to be preserved after primary stages.

3. Multiple postprocessing stages:
    Primary draws are made to the backbuffer of PPDB (no swapping).
    The postprocessing draws are made to the PPDB backbuffers, until
    the last draw, whic is made to the default backbuffer.


This is inflexible, as the screen cannot be sampled in the primary stages.
Will have to think about it more.

TODO: This is also not how it's implemented currently, as the primary draws
are always made to the "main_target", and then blitted to either
the PPDB backbuffer or the default framebuffer depending on the presence
of postprocessing. Regardless, this is wasteful, redo with corrections from
above.


*/
class RenderEngine {
public:
    friend RenderEngineCommonInterface;
    friend RenderEnginePrimaryInterface;
    friend RenderEnginePostprocessInterface;

private:
    entt::registry& registry_;

    Camera& cam_;
    const WindowSize<int>& window_size_;
    const FrameTimer& frame_timer_;

    std::vector<detail::AnyPrimaryStage> stages_;
    size_t current_stage_{};

    mutable RenderTargetColorAndDepth main_target_{
        window_size_.width, window_size_.height,
        gl::GL_RGBA, gl::GL_RGBA16F, gl::GL_FLOAT
    };

    PostprocessRenderer pp_renderer_;
    PostprocessDoubleBuffer ppdb_{
        window_size_.width, window_size_.height,
        gl::GL_RGBA, gl::GL_RGBA16F, gl::GL_FLOAT
    };
    size_t current_pp_stage_{};
    std::vector<detail::AnyPostprocessStage> pp_stages_;


public:
    RenderEngine(entt::registry& registry, Camera& cam,
        const WindowSize<int>& window_size, const FrameTimer& frame_timer)
        : registry_{ registry }
        , cam_{ cam }
        , window_size_{ window_size }
        , frame_timer_{ frame_timer }
    {}

    void render();

    template<primary_render_stage StageT, typename ...Args>
    [[nodiscard]] PrimaryStage<StageT> make_primary_stage(Args&&... args) {
        return PrimaryStage<StageT>(StageT(std::forward<Args>(args)...));
    }

    template<postprocess_render_stage StageT, typename ...Args>
    [[nodiscard]] PostprocessStage<StageT> make_postprocess_stage(Args&&... args) {
        return PostprocessStage<StageT>(StageT(std::forward<Args>(args)...));
    }

    template<typename StageT>
    StageT& add_next_primary_stage(PrimaryStage<StageT>&& stage) {
        // A lot of this relies on pointer/storage stability of UniqueFunction.
        StageT& ref = stage.target();
        stages_.emplace_back(detail::AnyPrimaryStage(std::move(stage.stage_)));
        return ref;
    }

    template<typename StageT>
    StageT& add_next_postprocess_stage(PostprocessStage<StageT>&& stage) {
        StageT& ref = stage.target();
        pp_stages_.emplace_back(detail::AnyPostprocessStage(std::move(stage.stage_)));
        return ref;
    }

    auto& camera() noexcept { return cam_; }
    const auto& camera() const noexcept { return cam_; }

    const WindowSize<int>& window_size() const noexcept { return window_size_; }
    const FrameTimer& frame_timer() const noexcept { return frame_timer_; }

    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        main_target_.reset_size(width, height);
        ppdb_.reset_size(width, height);
    }

    void reset_size_from_window_size() {
        auto [w, h] = window_size_;
        reset_size(w, h);
    }

private:
    void render_primary_stages();
    void render_postprocess_stages();
};








class RenderEngineCommonInterface {
protected:
    RenderEngine& engine_;

public:
    RenderEngineCommonInterface(RenderEngine& engine) : engine_{ engine } {}

    const Camera& camera() const noexcept { return engine_.cam_; }
    const WindowSize<int>& window_size() const noexcept { return engine_.window_size_; }
    const FrameTimer& frame_timer() const noexcept { return engine_.frame_timer_; }
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
        using namespace gl;

        engine_.main_target_.framebuffer().bind_as(GL_DRAW_FRAMEBUFFER)
            .and_then([&] {
                std::invoke(std::forward<CallableT>(draw_func), std::forward<Args>(args)...);
            })
            .unbind();

    }
};








class RenderEnginePostprocessInterface : public RenderEngineCommonInterface {
private:
    mutable size_t draw_call_budget_{ 1 };
    friend class RenderEngine;
    RenderEnginePostprocessInterface(RenderEngine& engine)
        : RenderEngineCommonInterface(engine)
    {}


public:
    PostprocessRenderer& postprocess_renderer() const {
        return engine_.pp_renderer_;
    }

    const Texture2D& screen_color() const {
        return engine_.ppdb_.front_target();
    }

    const Texture2D& screen_depth() const {
        return engine_.main_target_.depth_target();
    }

    // Emit the draw call on the screen quad and adjust the render target state
    // for the next stage in the chain.
    //
    // Note that the draw() can only be called once per stage for multiple reasons:
    //   1. Due to potential optimizations of the postprocessing chain. For example,
    //   last stage being drawn onto the default backbuffer instead of
    //   the swapchain backbuffer to save on a blit operation.
    //   2. Due to invalidation of the color and depth screen texture references after
    //   a draw call. This is just confusing.
    //
    // This design is subject to change in the future, however.
    void draw() const {
        assert(draw_call_budget_ && "draw() called more than once in a postprocessing stage.");

        using namespace gl;

        if (engine_.current_pp_stage_ < engine_.pp_stages_.size() - 1) {
            engine_.ppdb_.draw_and_swap([this] {
                engine_.pp_renderer_.draw();
            });
        } else /* last stage */ {
            // Draw to the screen directly.
            BoundFramebuffer::unbind_as(GL_DRAW_FRAMEBUFFER);
            engine_.pp_renderer_.draw();
        }

        --draw_call_budget_;
    }
};





} // namespace learn
