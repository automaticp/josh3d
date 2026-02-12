#pragma once
#include "EnumUtils.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "Region.hpp"
#include "ShaderPool.hpp"
#include "StaticRing.hpp"
#include "UniformTraits.hpp"
#include "StageContext.hpp"
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
TODO: Aliasing at fractional resolutions is still a PITA. Differient filtering
options have their own downside and no perfect solution. We might try blurring
at screen resolution, but it's super expensive. Figure something out.

TODO: We likely want determenistic a RNG for seed serialization.
*/
struct SSAO
{
    // Switch to false if you want to skip the whole stage.
    bool  enable_sampling    = true;
    // Used to scale the Occlusion buffer resolution compared to screen size.
    float resolution_divisor = 2.f;
    // Sampling kernel radius in world units.
    float radius             = 0.2f;
    float bias               = 0.01f;

    auto kernel_size() const noexcept -> usize { return _kernel.num_staged(); }
    // Minimum allowed angle between the surface and each kernel vector.
    auto deflection_rad() const noexcept -> float { return _deflection_rad; }
    void regenerate_kernel(usize kernel_size, float deflection_rad);

    enum class NoiseMode
    {
        SampledFromTexture,   // Has tiling artifacts, but is much easier to filter.
        GeneratedInShaderPCG, // Seeded from screen coordinates. Very unstable - needs aggressive filtering. Not recommended.
    };

    NoiseMode noise_mode = NoiseMode::SampledFromTexture;

    // Lower resolutions are easier to filter with a blur pass.
    // The sweet-spot is somewhere between 2x2 and 4x4.
    // YMMV depending on the current resolution_divisor and limb_size.
    auto noise_texture_resolution() -> Extent2I { return _noise_texture->get_resolution(); }
    void regenerate_noise_texture(Extent2I resolution);

    enum class BlurMode
    {
        Box,       // Very dumb 5x5 box blur. Expect severe "halo" artifacts. But cheap as beer.
        Bilateral, // Bilateral (depth-aware) gaussian blur.
    };

    BlurMode blur_mode       = BlurMode::Bilateral;
    // World-space depth difference limit for Bilateral blur. Should be close to radius.
    float    depth_limit     = 0.15f;
    // Only for Bilateral blur. Values above 1 *can* erase much wider artifacts
    // (this is referred to as blur's "aggressiveness"), but are not recommended due to
    // the severe performance impact it has. Try lowering the noise texture resolution instead.
    usize    num_blur_passes = 1;
    // For an NxN kernel, limb size L = (N - 1) / 2.
    auto blur_kernel_limb_size() const noexcept -> usize;
    // Blur kernel limb size of 0 effectively disables the blur.
    void resize_blur_kernel(usize limb_size);

    SSAO(
        usize    kernel_size              = 9,
        float    deflection_rad           = glm::radians(5.0f),
        Extent2I noise_texture_resolution = { 3, 3 },
        usize    blur_kernel_limb_size    = 2);

    void operator()(PrimaryContext context);

    AOBuffers aobuffers;


    UniqueFramebuffer _fbo;

    // NOTE: We use vec4 to avoid issues with alignment in std430,
    // even though we only need vec3 of data.
    UploadBuffer<vec4> _kernel;
    float              _deflection_rad = {};

    UniqueTexture2D _noise_texture;

    UniqueSampler _depth_sampler = create_sampler({
        // Somehow, setting this to Linear and the mag_filter
        // to Nearest summons the "triangle of doom". Still no idea why.
        // Something is very wrong here, and that kind of artifact
        // should not happen period.
        .min_filter = MinFilter::Linear,
        // Required to be linear to avoid various artifacts at high z.
        // At the same time, the upclose geometry has less aliasing
        // when this is nearest. Go figure.
        .mag_filter = MagFilter::Linear,
        .wrap_all   = Wrap::ClampToEdge,
    });

    UniqueSampler _normals_sampler = create_sampler({
        // NOTE: Linear is best against aliasing here.
        .min_filter = MinFilter::Linear,
        .mag_filter = MagFilter::Linear,
        .wrap_all   = Wrap::ClampToEdge,
    });

    UniqueSampler _blur_sampler = create_sampler({
        .min_filter = MinFilter::Linear,
        .mag_filter = MagFilter::Linear,
        .wrap_all   = Wrap::ClampToEdge,
    });

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
JOSH3D_DEFINE_ENUM_EXTRAS(SSAO::NoiseMode, GeneratedInShaderPCG, SampledFromTexture);
JOSH3D_DEFINE_ENUM_EXTRAS(SSAO::BlurMode, Box, Bilateral);


} // namespace josh
