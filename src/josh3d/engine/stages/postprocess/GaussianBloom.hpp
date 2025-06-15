#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "Scalars.hpp"
#include "ShaderPool.hpp"
#include "StaticRing.hpp"
#include "UniformTraits.hpp"
#include "RenderEngine.hpp"
#include "Region.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"


namespace josh {


struct GaussianBloomTarget
{
    auto resolution() const noexcept -> Extent2I { return _resolution; }
    auto front_texture() const noexcept -> RawTexture2D<> { return _swapchain.current().texture; }

    static constexpr auto iformat = InternalFormat::R11F_G11F_B10F;

    struct Side
    {
        UniqueTexture2D   texture;
        UniqueFramebuffer fbo;
    };
    Extent2I            _resolution = { 0, 0 };
    StaticRing<Side, 2> _swapchain;
    void _resize(Extent2I resolution);
    auto _front() noexcept -> Side& { return _swapchain.current(); }
    auto _back()  noexcept -> Side& { return _swapchain.next(); }
    void _swap()  noexcept { _swapchain.advance(); }
};

inline void GaussianBloomTarget::_resize(Extent2I resolution)
{
    if (resolution != _resolution)
    {
        _resolution = resolution;
        for (auto& side : _swapchain._storage)
        {
            side.texture = {};
            side.texture->allocate_storage(_resolution, iformat);
            side.fbo->attach_texture_to_color_buffer(side.texture, 0);
        }
    }
}


/*
Old gaussian bloom implementation. Slow and not pretty.
*/
struct GaussianBloom
{
    bool  use_bloom        = true;
    vec2  threshold_bounds = { 0.05f, 1.0f };
    usize blur_iterations  = 1;
    float offset_scale     = 1.f;

    // The underlying gaussian is sampled N times in [-range, +range],
    // where N=2*(L+1) is the size of the kernel, and L is the limb size.
    auto kernel_range() const noexcept -> float { return kernel_range_; }

    // For a kernel of size N, the limb size is (N-1)/2.
    auto kernel_limb_size() const noexcept -> usize;

    void resize_kernel(usize limb_size, float range);

    GaussianBloom(usize kernel_limb_size = 2, float kernel_range = 3.13f);

    void operator()(RenderEnginePostprocessInterface& engine);

    GaussianBloomTarget target;

private:
    float               kernel_range_ = 1.f;
    UploadBuffer<float> kernel_weights_;

    UniqueSampler sampler_ = eval%[]{
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    };

    ShaderToken sp_extract_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_bloom_threshold_extract.frag")});

    ShaderToken sp_twopass_gaussian_blur_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_bloom_twopass_gaussian_blur.frag")});

    ShaderToken sp_blend_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_bloom_blend.frag")});
};


} // namespace josh
