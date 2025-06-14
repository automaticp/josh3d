#pragma once
#include "Attachments.hpp"
#include "Belt.hpp"
#include "ECS.hpp"
#include "EnumUtils.hpp"
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
#include "Skeleton.hpp"
#include "SwapChain.hpp"
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

enum class HDRFormat : GLuint
{
    R11F_G11F_B10F  = GLuint(InternalFormat::R11F_G11F_B10F),
    RGB16F          = GLuint(InternalFormat::RGB16F),
    RGB32F          = GLuint(InternalFormat::RGB32F), // Don't know why you'd want this but...
};
JOSH3D_DEFINE_ENUM_EXTRAS(HDRFormat, R11F_G11F_B10F, RGB16F, RGB32F);

/*
FIXME: This whole thing is pretty useless. It's just a pipeline
with a main buffer and some conveniences; and a lot of friction.
*/
class RenderEngine
{
public:
    HDRFormat main_buffer_format;
    Extent2I  main_resolution;

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
        HDRFormat main_format);

    void render(
        Registry&           registry,
        const MeshRegistry& mesh_registry,
        const Primitives&   primitives,
        const Extent2I&     window_resolution,
        const FrameTimer&   frame_timer);

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

    auto main_depth_texture()    const noexcept -> RawTexture2D<GLConst> { return main_depth_.texture(); }
    auto main_depth_attachment() const noexcept -> const auto&           { return main_depth_;           }
    auto share_main_depth_attachment() noexcept -> auto                  { return main_depth_.share();   }

    auto main_color_texture()    const noexcept -> RawTexture2D<GLConst> { return main_swapchain_.back_target().color_attachment().texture(); }
    auto main_color_attachment() const noexcept -> const auto&           { return main_swapchain_.back_target().color_attachment();           }
    auto share_main_color_attachment() noexcept -> auto                  { return main_swapchain_.back_target().share_color_attachment();     }

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

    using MainTarget = RenderTarget<
        SharedAttachment<Renderable::Texture2D>,    // Depth
        ShareableAttachment<Renderable::Texture2D>  // Color
    >;
    using DepthAttachment = ShareableAttachment<Renderable::Texture2D>;

    static auto make_main_depth(const Size2I& resolution)
        -> DepthAttachment;

    static auto make_main_swapchain(
        const Size2I&    resolution,
        InternalFormat   iformat,
        DepthAttachment& depth)
            -> SwapChain<MainTarget>;

    DepthAttachment       main_depth_;
    SwapChain<MainTarget> main_swapchain_;

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
    UniqueBuffer<CameraDataGPU> camera_ubo_{ allocate_buffer<CameraDataGPU>(NumElems{ 1 }) };

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
    auto main_resolution() const noexcept -> const Size2I&     { return _engine.main_resolution; }
    auto frame_timer()     const noexcept -> const FrameTimer& { return _frame_timer;    }

    auto bind_camera_ubo(GLuint index = 0) const noexcept
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
        BindGuard bound_fbo = _engine.main_swapchain_.back_target().bind_draw();
        draw_func(bound_fbo.token());
    }

    // The RenderEngine's main framebuffer is not exposed here because
    // there's a high chance you'll be reading from it, as you're writing
    // to it. Hear me? You! Yes, you. Think twice before implementing it again.
    //
    // I didn't allow it initially for a reason.

    // FIXME: Unscrew this on the main rendertarget level.
    auto main_depth_texture() noexcept -> RawTexture2D<> { return RawTexture2D<>::from_id(_engine.main_depth_texture().id()); }
    auto main_color_texture() noexcept -> RawTexture2D<> { return RawTexture2D<>::from_id(_engine.main_color_texture().id()); }

    auto main_depth_attachment() const noexcept -> const auto& { return _engine.main_depth_attachment();       }
    auto share_main_depth_attachment() noexcept -> auto        { return _engine.share_main_depth_attachment(); }

    auto main_color_attachment() const noexcept -> const auto& { return _engine.main_color_attachment();       }
    auto share_main_color_attachment() noexcept -> auto        { return _engine.share_main_color_attachment(); }
};

struct RenderEnginePostprocessInterface : RenderEngineCommonInterface
{
    auto screen_color() const noexcept -> RawTexture2D<GLConst>
    { return _engine.main_swapchain_.front_target().color_attachment().texture(); }

    auto screen_depth() const noexcept -> RawTexture2D<GLConst>
    { return _engine.main_swapchain_.front_target().depth_attachment().texture(); }

    // Emit the draw call on the screen quad and adjust the render target state
    // for the next stage in the chain.
    //
    // The screen color texture is INVALIDATED for sampling after this call.
    // You have to call screen_color() again and bind the returned texture
    // in order to sample the screen in the next call to draw().
    void draw(BindToken<Binding::Program> bound_program)
    {
        _engine.main_swapchain_.draw_and_swap([&](BindToken<Binding::DrawFramebuffer> bound_fbo)
        {
            primitives().quad_mesh().draw(bound_program, bound_fbo);
        });
    }

    // Emit the draw call on the screen quad and draw directly to the front buffer.
    // DOES NOT advance the chain. You CANNOT SAMPLE THE SCREEN COLOR during this draw.
    //
    // Used as an optimization for draws that either override or blend with the screen.
    void draw_to_front(BindToken<Binding::Program> bound_program)
    {
        BindGuard bound_fbo{ _engine.main_swapchain_.front_target().bind_draw() };
        primitives().quad_mesh().draw(bound_program, bound_fbo);
    }
};

struct RenderEngineOverlayInterface : RenderEngineCommonInterface
{
    // Emit the draw call on the screen quad and draw directly to the default buffer.
    void draw_fullscreen_quad(BindToken<Binding::Program> bound_program)
    {
        BindGuard bound_fbo{ _engine.default_fbo_.bind_draw() };
        primitives().quad_mesh().draw(bound_program, bound_fbo);
    }

    // Effectively binds the default framebuffer as the Draw framebuffer
    // and invokes the callable argument.
    //
    // Note that it is illegal to bind any framebuffer object as
    // a Draw framebuffer within the callable.
    template<std::invocable<BindToken<Binding::DrawFramebuffer>> CallableT>
    void draw(CallableT&& draw_func)
    {
        draw_func(_engine.default_fbo_.bind_draw());
    }
};


} // namespace josh
