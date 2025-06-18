#pragma once
#include "EnumUtils.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "Region.hpp"
#include "ShaderPool.hpp"
#include "StaticRing.hpp"
#include "UniformTraits.hpp"
#include "RenderEngine.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"
#include <glm/trigonometric.hpp>


namespace josh {


struct AOBuffers
{
    auto resolution()      const noexcept -> Extent2I { return _resolution; }
    [[deprecated]]
    auto noisy_texture()   const noexcept -> RawTexture2D<> { return backbuffer_texture(); }
    // Depending on the blur mode and resolution divisors we could do extra
    // swaps between the passes, this will always end up the as the "back" buffer
    // but it does not guarantee that it contains "pure" original samples.
    auto backbuffer_texture()      const noexcept -> RawTexture2D<> { return _swapchain.next().texture;    }
    auto occlusion_texture() const noexcept -> RawTexture2D<> { return _swapchain.current().texture; }

    static constexpr auto iformat = InternalFormat::R8;

    struct Side
    {
        UniqueTexture2D texture;
    };

    Extent2I            _resolution = { 0, 0 };
    StaticRing<Side, 2> _swapchain;
    void _resize(Extent2I new_resolution);
    auto _front() noexcept -> Side& { return _swapchain.current(); }
    auto _back()  noexcept -> Side& { return _swapchain.next(); }
    void _swap()  noexcept          { _swapchain.advance(); }
};

inline void AOBuffers::_resize(Extent2I new_resolution)
{
    if (_resolution == new_resolution) return;
    _resolution = new_resolution;
    for (Side& side : _swapchain.storage)
    {
        side.texture = {};
        side.texture->allocate_storage(_resolution, iformat);
    }
}

/*
FIXME: The aliasing is pretty bad when sampling at fractional resolution:
- "Dark" aliased edges around objects at ~same occlusion level with large depth-delta.
- "Bright" aliased edges next at hi->lo occlusion depth-discontinuous edges.

FIXME: The diagonal line of doom haunts me when using noise texture as the source.
I don't remember what's causing it. Sampler state? Divergent texture() calls?
Maybe I never figured it out?
*/
struct SSAO
{
    bool  enable_sampling    = true;  // Switch to false if you want to skip the whole stage.
    float radius             = 0.2f;  // Sampling kernel radius in world units.
    float bias               = 0.01f; // Can't wait to do another reciever plane bias...
    float resolution_divisor = 2.f;   // Used to scale the Occlusion buffer resolution compared to screen size.

    auto kernel_size() const noexcept -> usize { return _kernel.num_staged(); }
    // Minimum allowed angle between the surface and each kernel vector.
    auto deflection_rad() const noexcept -> float { return _deflection_rad; }
    void regenerate_kernel(usize kernel_size, float deflection_rad);

    enum class NoiseMode
    {
        SampledFromTexture, // Has tiling artifacts, lower resolution is easier to filter.
        GeneratedInShader,  // Using basic PCG. Quite noisy and unstable. Needs aggressive filtering.
    };

    NoiseMode noise_mode = NoiseMode::SampledFromTexture;

    auto noise_texture_resolution() -> Extent2I { return _noise_texture->get_resolution(); }
    void regenerate_noise_texture(Extent2I resolution);

    enum class BlurMode
    {
        Box,       // Very naive box blur.
        Bilateral, // Bilateral (depth-aware) gaussian blur.
    };

    BlurMode blur_mode       = BlurMode::Bilateral;
    float    depth_limit     = 0.15f; // World-space depth difference limit for Bilateral blur. Should be close to radius.
    usize    num_blur_passes = 1;     // Only for Bilateral blur. Values above 1 are not recommended due to performance reasons.

    auto blur_kernel_limb_size() const noexcept -> usize;
    // Blur kernel limb size of 0 effectively disables the blur.
    void resize_blur_kernel(usize limb_size);

    SSAO(
        usize    kernel_size              = 9,
        float    deflection_rad           = glm::radians(5.0f),
        Extent2I noise_texture_resolution = { 4, 4 },
        usize    blur_kernel_limb_size    = 2);

    void operator()(RenderEnginePrimaryInterface& engine);

    AOBuffers aobuffers;

private:
    UniqueFramebuffer _fbo;

    // NOTE: We use vec4 to avoid issues with alignment in std430,
    // even though we only need vec3 of data.
    UploadBuffer<vec4> _kernel;
    float              _deflection_rad = {};

    UniqueTexture2D _noise_texture;

    UniqueSampler _target_sampler = eval%[]{
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    };

    ShaderToken _sp_sampling = shader_pool().get({
        .vert = VPath("src/shaders/screen_quad.vert"),
        .frag = VPath("src/shaders/ssao_sampling.frag")});

    ShaderToken _sp_blur_box = shader_pool().get({
        .vert = VPath("src/shaders/screen_quad.vert"),
        .frag = VPath("src/shaders/ssao_blur_box.frag")});

    UploadBuffer<float> _blur_kernel;

    ShaderToken _sp_blur_bilateral = shader_pool().get({
        .vert = VPath("src/shaders/screen_quad.vert"),
        .frag = VPath("src/shaders/ssao_blur_bilateral.frag")});
};
JOSH3D_DEFINE_ENUM_EXTRAS(SSAO::NoiseMode, GeneratedInShader, SampledFromTexture);
JOSH3D_DEFINE_ENUM_EXTRAS(SSAO::BlurMode, Box, Bilateral);


} // namespace josh
