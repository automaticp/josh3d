#pragma once
#include "Belt.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "GLTextures.hpp"
#include "HashedString.hpp"
#include "MeshRegistry.hpp"
#include "PerfHarness.hpp"
#include "Primitives.hpp"
#include "RenderEngine.hpp"
#include "Runtime.hpp"


namespace josh {


/*
Context for rendering stages. Passed to each stage from the RenderEngine.

NOTE: These are thin wrappers around a pointer and should be taken by value.
Each concrete context only differs by member functions it exposes.
*/
struct StageContext
{
    // Communication channel between stages and with the outside world.
    auto belt()              const noexcept -> Belt&               { return _state.engine.belt;           }
    auto mesh_registry()     const noexcept -> const MeshRegistry& { return _state.runtime.mesh_registry; }

    // Take an extra perf snapshot in the middle of the current stage.
    // The name could be anything other than the reserved "start"and "end".
    // If no harness is attached, this is a no-op.
    template<usize N>
    void perf_snap(FixedHashedString<N> name_hs) const noexcept
    { if (_stage_state.perf_harness) _stage_state.perf_harness->take_snap(name_hs); }

    // NOTE: Normally, the registry is considered read-only during rendering,
    // but if you know what you are doing, you can explicitly request a mutable
    // reference to it. Beware that this should likely not modify existing
    // components defined outside of the stage's purview.
    //
    // This will also be widely used in precompute stages. That is OK.
    auto mutable_registry()  const noexcept -> Registry&           { return _state.runtime.registry;      }
    auto registry()          const noexcept -> const Registry&     { return _state.runtime.registry;      }

    auto primitives()        const noexcept -> const Primitives&   { return _state.primitives;            }
    auto main_resolution()   const noexcept -> Extent2I            { return _state.engine.main_resolution();      }
    auto window_resolution() const noexcept -> Extent2I            { return _state.window_resolution;     }
    auto frame_timer()       const noexcept -> const FrameTimer&   { return _state.frame_timer;           }
    auto camera_data()       const noexcept -> const auto&         { return _state.engine._camera_data;           }
    auto bind_camera_ubo(u32 index = 0) const noexcept -> BindToken<BindingI::UniformBuffer>
    { return _state.engine._camera_ubo->bind_to_index<BufferTargetI::Uniform>(index); }

    auto main_depth_texture()       const noexcept -> RawTexture2D<>        { return _state.engine._main_target.depth();       }
    auto main_back_color_texture()  const noexcept -> RawTexture2D<>        { return _state.engine._main_target.back_color();  }
    auto main_front_color_texture() const noexcept -> RawTexture2D<GLConst> { return _state.engine._main_target.front_color(); }

    struct CommonState
    {
        RenderEngine&     engine;
        Runtime&          runtime;
        const Primitives& primitives;
        const FrameTimer& frame_timer;
        Extent2I          window_resolution;
    };

    struct PerStageState
    {
        PerfHarness* perf_harness = nullptr;
    };

    CommonState&   _state;
    PerStageState& _stage_state;
};


/*
Precompute stages do computations that are exlusively related to rendering,
but do not use gpu primitives (ex. frustum culling, buffer setup, etc.).

NOTE: No extra functions for precompute.
*/
JOSH3D_DERIVE_TYPE(PrecomputeContext, StageContext);

/*
Primary stages perform geometry and shading passes and
*may* write to the back color of the main render target.
*/
JOSH3D_DERIVE_TYPE_EX(PrimaryContext, StageContext)
//{
    // Effectively binds the main render target as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> F>
    void bind_back_and(F&& draw_func)
    {
        const BindGuard bfb = _state.engine._main_target._back().fbo->bind_draw();
        draw_func(bfb.token());
    }
};

/*
Postprocessing stages work on the resulting image produced from
the primary passes, but may also pull information from other products
(ex. GBuffer, IDBuffer, AOBuffers, etc).

The main color target is double-buffered, so ping-ponging is possible
in the postprocessing stages.
*/
JOSH3D_DERIVE_TYPE_EX(PostprocessContext, StageContext)
//{
    // Emit the draw call on the screen quad and adjust the render target state
    // for the next stage in the chain.
    //
    // The front color texture is INVALIDATED for sampling after this call.
    // You have to call main_color_front() again and bind the returned texture
    // in order to sample the screen in the next call to draw*().
    void draw_quad_and_swap(BindToken<Binding::Program> bsp)
    {
        const BindGuard bfb = _state.engine._main_target._back().fbo->bind_draw();
        primitives().quad_mesh().draw(bsp, bfb);
        _state.engine._main_target._swap();
    }

    // Emit the draw call on the screen quad and draw directly to the front buffer.
    // DOES NOT advance the chain.
    //
    // WARNING: You CANNOT SAMPLE THE FRONT COLOR during this draw.
    //
    // Used as an optimization for draws that either override or blend with the screen.
    void draw_quad_to_front(BindToken<Binding::Program> bsp)
    {
        const BindGuard bound_fbo = _state.engine._main_target._front().fbo->bind_draw();
        primitives().quad_mesh().draw(bsp, bound_fbo);
    }
};

/*
Overlay stages write to the default framebuffer and are intended for
UI and various debug overlays.
*/
JOSH3D_DERIVE_TYPE_EX(OverlayContext, StageContext)
//{
    // Emit the draw call on the screen quad and draw directly to the default buffer.
    void draw_quad_to_default(BindToken<Binding::Program> bsp)
    {
        const BindGuard bfb = _state.engine._default_fbo.bind_draw();
        primitives().quad_mesh().draw(bsp, bfb);
    }

    // Effectively binds the default framebuffer as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> CallableT>
    void bind_default_and(CallableT&& draw_func)
    {
        const BindGuard bfb = _state.engine._default_fbo.bind_draw();
        draw_func(bfb.token());
    }
};


} // namespace josh
