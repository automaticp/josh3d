#pragma once
#include "Camera.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "PostprocessDoubleBuffer.hpp"
#include "PostprocessRenderer.hpp"
#include "RenderTargetColorAndDepth.hpp"
#include "UniqueFunction.hpp"
#include "WindowSize.hpp"
#include <entt/fwd.hpp>
#include <glbinding/gl/gl.h>
#include <cassert>
#include <concepts>
#include <functional>
#include <vector>


// WIP



namespace learn {
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
private:
    /*
    Implementation base for wrapper types that constrain the actions avaliable
    to be done with the engine during primary and postprocessing stages.
    */
    class InterfaceCommon;

public:
    /*
    A wrapper object that constrains the set of actions avaliable
    during primary stages. Passed to the primary stages
    as a proxy for RenderEngine.
    */
    class PrimaryInterface;

    /*
    A wrapper object that constrains the set of actions avaliable
    during postprocessing stages. Passed to the postprocessing stages
    as a proxy for RenderEngine.
    */
    class PostprocessInterface;


    using primary_stage_t =
        UniqueFunction<void(const RenderEngine::PrimaryInterface&, const entt::registry&)>;

    using postprocess_stage_t =
        UniqueFunction<void(const RenderEngine::PostprocessInterface&, const entt::registry&)>;

private:
    entt::registry& registry_;

    Camera& cam_;
    const WindowSize<int>& window_size_;
    const FrameTimer& frame_timer_;

    std::vector<primary_stage_t> stages_;
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
    std::vector<postprocess_stage_t> pp_stages_;


public:
    RenderEngine(entt::registry& registry, Camera& cam,
        const WindowSize<int>& window_size, const FrameTimer& frame_timer)
        : registry_{ registry }
        , cam_{ cam }
        , window_size_{ window_size }
        , frame_timer_{ frame_timer }
    {}

    void render();

    auto& stages() noexcept { return stages_; }
    const auto& stages() const noexcept { return stages_; }

    auto& postprocess_stages() noexcept { return pp_stages_; }
    const auto& postprocess_stages() const noexcept { return pp_stages_; }

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




template<typename StageT>
concept primary_render_stage = std::invocable<StageT, const RenderEngine::PrimaryInterface&, const entt::registry&>;

template<typename StageT>
concept postprocess_render_stage = std::invocable<StageT, const RenderEngine::PostprocessInterface&, const entt::registry&>;








class RenderEngine::InterfaceCommon {
protected:
    RenderEngine& engine_;

public:
    InterfaceCommon(RenderEngine& engine) : engine_{ engine } {}

    const Camera& camera() const noexcept { return engine_.cam_; }
    const WindowSize<int>& window_size() const noexcept { return engine_.window_size_; }
    const FrameTimer& frame_timer() const noexcept { return engine_.frame_timer_; }
};








class RenderEngine::PrimaryInterface : public InterfaceCommon {
private:
    friend class RenderEngine;
    PrimaryInterface(RenderEngine& engine) : InterfaceCommon(engine) {}

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








class RenderEngine::PostprocessInterface : public InterfaceCommon {
private:
    mutable size_t draw_call_budget_{ 1 };
    friend class RenderEngine;
    PostprocessInterface(RenderEngine& engine) : InterfaceCommon(engine) {}


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
