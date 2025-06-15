#pragma once
#include "Belt.hpp"
#include "ECS.hpp"
#include "EnumUtils.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "GLObjectHelpers.hpp"
#include "GLTextures.hpp"
#include "GPULayout.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "MeshRegistry.hpp"
#include "Primitives.hpp"
#include "Region.hpp"
#include "RenderStage.hpp"
#include "Skeleton.hpp"
#include "StaticRing.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <ranges>
#include <vector>


namespace josh {


/*
Implementation base for wrapper types that constrain the actions avaliable
to be done with the engine during various stages.
*/
struct RenderEngineCommonInterface;

/*
A set of wrapper objects that constrains the set of actions avaliable
during specified stages. Passed to the the stages as a proxy for RenderEngine.
*/
struct RenderEnginePrecomputeInterface;
struct RenderEnginePrimaryInterface;
struct RenderEnginePostprocessInterface;
struct RenderEngineOverlayInterface;

enum class HDRFormat : u32
{
    R11F_G11F_B10F  = u32(InternalFormat::R11F_G11F_B10F),
    RGB16F          = u32(InternalFormat::RGB16F),
    RGB32F          = u32(InternalFormat::RGB32F), // Don't know why you'd want this but...
};
JOSH3D_DEFINE_ENUM_EXTRAS(HDRFormat, R11F_G11F_B10F, RGB16F, RGB32F);

/*
NOTE: Currently *need* stencil for some operations, so the choice is slim.
*/
enum class DSFormat : u32
{
    Depth24_Stencil8 = u32(InternalFormat::Depth24_Stencil8),
    // No, does not work currently as it is impossible to blit from
    // the floating point depth to the default fbo. We'd need
    // a custom blit shader for that then... Meh.
    // Depth32F_Stencil8 = u32(InternalFormat::Depth32F_Stencil8),
};
JOSH3D_DEFINE_ENUM_EXTRAS(DSFormat, Depth24_Stencil8);

struct MainTarget
{
    auto resolution()    const noexcept -> Extent2I  { return _resolution; }
    auto color_iformat() const noexcept -> HDRFormat { return _iformat_color; }
    auto depth_iformat() const noexcept -> DSFormat  { return _iformat_depth; }
    auto depth()         const noexcept -> RawTexture2D<> { return _depth; }
    auto front_color()   const noexcept -> RawTexture2D<> { return _swapchain.current().color; }
    auto back_color()    const noexcept -> RawTexture2D<> { return _swapchain.next().color; }

    struct Side
    {
        UniqueTexture2D   color;
        UniqueFramebuffer fbo;
    };
    Extent2I            _resolution    = { 0, 0 };
    HDRFormat           _iformat_color = HDRFormat::R11F_G11F_B10F;
    DSFormat            _iformat_depth = DSFormat::Depth24_Stencil8;
    UniqueTexture2D     _depth; // Shared between front and back sides.
    StaticRing<Side, 2> _swapchain;
    void _respec(Extent2I resolution, HDRFormat iformat_color, DSFormat iformat_depth);
    auto _front() noexcept -> Side& { return _swapchain.current(); }
    auto _back()  noexcept -> Side& { return _swapchain.next();    }
    void _swap()  noexcept { _swapchain.advance(); }
};

inline void MainTarget::_respec(
    Extent2I  resolution,
    HDRFormat iformat_color,
    DSFormat  iformat_depth)
{
    // Handle depth separately, since it does not care about color format changes.
    if (resolution != _resolution or
        iformat_depth != _iformat_depth)
    {
        _depth = {};
        _depth->allocate_storage(resolution, enum_cast<InternalFormat>(iformat_depth));
        for (auto& side : _swapchain.storage)
            side.fbo->attach_texture_to_depth_buffer(_depth);
    }

    if (resolution != _resolution or
        iformat_color != _iformat_color)
    {
        for (auto& side : _swapchain.storage)
        {
            side.color = {};
            side.color->allocate_storage(resolution, enum_cast<InternalFormat>(iformat_color));
            side.fbo->attach_texture_to_color_buffer(side.color, 0);
        }
    }

    _resolution    = resolution;
    _iformat_depth = iformat_depth;
    _iformat_color = iformat_color;
}


/*
FIXME: This whole thing is pretty useless. It's just a pipeline
with a main buffer and some conveniences; and a lot of friction.
*/
struct RenderEngine
{
    // Parameters of the main render target.
    auto main_resolution()   const noexcept -> Extent2I { return _main_target.resolution(); }
    auto main_depth_format() const noexcept -> DSFormat { return _main_target.depth_iformat(); }
    auto main_color_format() const noexcept -> HDRFormat { return _main_target.color_iformat(); }
    auto main_depth_texture() const noexcept -> RawTexture2D<> { return _main_target.depth(); }
    // TODO: Why is this BACK side?
    auto main_color_texture() const noexcept -> RawTexture2D<> { return _main_target.back_color(); }
    void respec_main_target(Extent2I resolution, HDRFormat color_iformat, DSFormat depth_iformat);

    // Enables RGB -> sRGB conversion at the end of the postprocessing pass.
    bool  enable_srgb_conversion = true;
    // Enables profiling of GPU/CPU times taken per each stage.
    bool  capture_stage_timings  = true;
    // The wall-time interval between updates of the timers, in seconds.
    // Note that the GPU timing is asyncronous and might lag behind by a frame or two.
    float stage_timing_averaging_interval_s = 0.5f;

    // Communication channel for pipeline stages. The belt is swept
    // in the beginning of the call to `render()`, *before* the pipeline
    // is executed. You are free to peek after.
    Belt belt;

    RenderEngine(
        Extent2I  main_resolution,
        HDRFormat main_color_format = HDRFormat::R11F_G11F_B10F,
        DSFormat  main_depth_format = DSFormat::Depth24_Stencil8);

    void render(
        Registry&           registry,
        const MeshRegistry& mesh_registry,
        const Primitives&   primitives,
        const Extent2I&     window_resolution,
        const FrameTimer&   frame_timer);

    MainTarget _main_target; // FIXME: Why is this "private"?

    auto precompute_stages_view()  noexcept { return std::views::all(precompute_);  }
    auto primary_stages_view()     noexcept { return std::views::all(primary_);     }
    auto postprocess_stages_view() noexcept { return std::views::all(postprocess_); }
    auto overlay_stages_view()     noexcept { return std::views::all(overlay_);   }

    template<precompute_render_stage StageT>
    void add_next_precompute_stage(String name, StageT&& stage)
    { precompute_.push_back({ MOVE(name), AnyPrecomputeStage{ FORWARD(stage) } }); }

    template<primary_render_stage StageT>
    void add_next_primary_stage(String name, StageT&& stage)
    { primary_.push_back({ MOVE(name), AnyPrimaryStage{ FORWARD(stage) } }); }

    template<postprocess_render_stage StageT>
    void add_next_postprocess_stage(String name, StageT&& stage)
    { postprocess_.push_back({ MOVE(name), AnyPostprocessStage{ FORWARD(stage) } }); }

    template<overlay_render_stage StageT>
    void add_next_overlay_stage(String name, StageT&& stage)
    { overlay_.push_back({ MOVE(name), AnyOverlayStage{ FORWARD(stage) } }); }

private:
    friend RenderEngineCommonInterface;
    friend RenderEnginePrecomputeInterface;
    friend RenderEnginePrimaryInterface;
    friend RenderEnginePostprocessInterface;
    friend RenderEngineOverlayInterface;

    Vector<PrecomputeStage>  precompute_;
    Vector<PrimaryStage>     primary_;
    Vector<PostprocessStage> postprocess_;
    Vector<OverlayStage>     overlay_;

    inline static const RawDefaultFramebuffer<GLMutable> default_fbo_;

    struct CameraDataGPU
    {
        alignas(std430::align_vec3)  vec3   position_ws; // World-space position
        alignas(std430::align_float) float  z_near;
        alignas(std430::align_float) float  z_far;
        alignas(std430::align_vec4)  mat4   view;
        alignas(std430::align_vec4)  mat4   proj;
        alignas(std430::align_vec4)  mat4   projview;
        alignas(std430::align_vec4)  mat4   inv_view;
        alignas(std430::align_vec4)  mat3x4 normal_view; // mat3, but padding is needed for each column in std140
        alignas(std430::align_vec4)  mat4   inv_proj;
        alignas(std430::align_vec4)  mat4   inv_projview;
    };

    CameraDataGPU               camera_data_;
    UniqueBuffer<CameraDataGPU> camera_ubo_ = allocate_buffer<CameraDataGPU>(1);

    void update_camera_data(
        const mat4& view,
        const mat4& proj,
        float       z_near,
        float       z_far) noexcept;
};


struct RenderEngineCommonInterface
{
    auto belt()               noexcept -> Belt&                 { return _engine.belt;   }
    auto meshes()       const noexcept -> const MeshRegistry&   { return _mesh_registry; }
    auto registry()     const noexcept -> const Registry&       { return _registry;      }
    auto primitives()   const noexcept -> const Primitives&     { return _primitives;    }
    auto camera_data()  const noexcept -> const auto&           { return _engine.camera_data_; }
    // TODO: Something about window resolution being separate?
    // TODO: Also main_resolution() is not accurate in Overlay stages.
    auto main_resolution() const noexcept -> Extent2I          { return _engine.main_resolution(); }
    auto frame_timer()     const noexcept -> const FrameTimer& { return _frame_timer;    }

    auto bind_camera_ubo(u32 index = 0) const noexcept
        -> BindToken<BindingIndexed::UniformBuffer>
    {
        return _engine.camera_ubo_->bind_to_index<BufferTargetIndexed::Uniform>(index);
    }

    RenderEngine&       _engine;
    Registry&           _registry;
    const MeshRegistry& _mesh_registry;
    const Primitives&   _primitives;
    const FrameTimer&   _frame_timer;
};

struct RenderEnginePrecomputeInterface : RenderEngineCommonInterface
{
    auto registry()       noexcept ->       Registry& { return _registry; }
    auto registry() const noexcept -> const Registry& { return _registry; }
};

struct RenderEnginePrimaryInterface : RenderEngineCommonInterface
{
    // Effectively binds the main render target as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> CallableT>
    void draw(CallableT&& draw_func)
    {
        const BindGuard bfb = _engine._main_target._back().fbo->bind_draw();
        draw_func(bfb.token());
    }

    // The RenderEngine's main framebuffer is not exposed here because
    // there's a high chance you'll be reading from it, as you're writing
    // to it. Hear me? You! Yes, you. Think twice before implementing it again.
    //
    // I didn't allow it initially for a reason.

    // FIXME: Unscrew this on the main rendertarget level.
    auto main_depth_texture() noexcept -> RawTexture2D<> { return _engine.main_depth_texture(); }
    auto main_color_texture() noexcept -> RawTexture2D<> { return _engine.main_color_texture(); }
};

struct RenderEnginePostprocessInterface : RenderEngineCommonInterface
{
    auto screen_color() const noexcept
        -> RawTexture2D<GLConst>
    {
        return _engine._main_target.front_color();
    }

    auto screen_depth() const noexcept
        -> RawTexture2D<GLConst>
    {
        return _engine._main_target.depth();
    }

    // Emit the draw call on the screen quad and adjust the render target state
    // for the next stage in the chain.
    //
    // The screen color texture is INVALIDATED for sampling after this call.
    // You have to call screen_color() again and bind the returned texture
    // in order to sample the screen in the next call to draw().
    void draw(BindToken<Binding::Program> bsp)
    {
        const BindGuard bfb = _engine._main_target._back().fbo->bind_draw();
        primitives().quad_mesh().draw(bsp, bfb);
        _engine._main_target._swap();
    }

    // Emit the draw call on the screen quad and draw directly to the front buffer.
    // DOES NOT advance the chain. You CANNOT SAMPLE THE SCREEN COLOR during this draw.
    //
    // Used as an optimization for draws that either override or blend with the screen.
    void draw_to_front(BindToken<Binding::Program> bsp)
    {
        const BindGuard bound_fbo = _engine._main_target._front().fbo->bind_draw();
        primitives().quad_mesh().draw(bsp, bound_fbo);
    }
};

struct RenderEngineOverlayInterface : RenderEngineCommonInterface
{
    // Emit the draw call on the screen quad and draw directly to the default buffer.
    void draw_fullscreen_quad(BindToken<Binding::Program> bsp)
    {
        const BindGuard bfb = _engine.default_fbo_.bind_draw();
        primitives().quad_mesh().draw(bsp, bfb);
    }

    // Effectively binds the default framebuffer as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> CallableT>
    void draw(CallableT&& draw_func)
    {
        const BindGuard bfb = _engine.default_fbo_.bind_draw();
        draw_func(bfb.token());
    }
};


} // namespace josh
