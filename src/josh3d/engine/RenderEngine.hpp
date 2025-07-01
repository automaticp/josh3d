#pragma once
#include "Belt.hpp"
#include "EnumUtils.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "GLObjectHelpers.hpp"
#include "GLTextures.hpp"
#include "GPULayout.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "Pipeline.hpp"
#include "Region.hpp"
#include "Skeleton.hpp"
#include "StaticRing.hpp"


namespace josh {

struct Runtime;

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


/*
HMM: After various refactorings, this was squeezed to be just a container
for the main render target and some parameters of the render() function.

If not for the special control flow in render(), this could simply be
the first primary pipeline stage.
*/
struct RenderEngine
{
    // Parameters of the main render target.
    auto main_resolution()   const noexcept -> Extent2I { return _main_target.resolution(); }
    auto main_depth_format() const noexcept -> DSFormat { return _main_target.depth_iformat(); }
    auto main_color_format() const noexcept -> HDRFormat { return _main_target.color_iformat(); }
    auto main_depth_texture() const noexcept -> RawTexture2D<> { return _main_target.depth(); }
    // FIXME: Why is this BACK side?
    auto main_color_texture() const noexcept -> RawTexture2D<> { return _main_target.back_color(); }
    void respec_main_target(Extent2I resolution, HDRFormat color_iformat, DSFormat depth_iformat);

    // Enables RGB -> sRGB conversion when blitting from main to  the destination
    // render target. This happens at the end of the postprocessing stages.
    bool enable_srgb_conversion = true;

    // Automatically resize the main target to window size on each call to render().
    bool fit_window_size = true;

    // Rendering stages that get executed on each call to render().
    // Assemble this after creating the engine itself.
    Pipeline pipeline;

    // Communication channel for pipeline stages. The belt is swept
    // in the beginning of the call to `render()`, *before* the pipeline
    // is executed. You are free to peek after.
    Belt belt;

    RenderEngine(
        Extent2I  main_resolution,
        HDRFormat main_color_format = HDRFormat::R11F_G11F_B10F,
        DSFormat  main_depth_format = DSFormat::Depth24_Stencil8);

    // HMM: This might as well be a free function with how it couples together a bunch of stuff.
    // TODO: Pass the destination FBO here? It needs to have depth+stencil and has other constraints...
    void render(
        Runtime&          runtime,
        const Extent2I&   window_resolution,
        const FrameTimer& frame_timer);

    MainTarget _main_target; // FIXME: Why is this "private"?

    // FIXME: This should be configurable, no? Just pass "destination" FBO to render?
    inline static const RawDefaultFramebuffer<GLMutable> _default_fbo = {};

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

    CameraDataGPU               _camera_data;
    UniqueBuffer<CameraDataGPU> _camera_ubo = allocate_buffer<CameraDataGPU>(1);

    void _update_camera_data(
        const mat4& view,
        const mat4& proj,
        float       z_near,
        float       z_far) noexcept;
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


} // namespace josh
