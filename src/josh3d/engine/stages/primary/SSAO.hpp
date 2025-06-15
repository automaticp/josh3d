#pragma once
#include "EnumUtils.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "Region.hpp"
#include "ShaderPool.hpp"
#include "UniformTraits.hpp"
#include "RenderEngine.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"
#include <glm/trigonometric.hpp>


namespace josh {


struct AOBuffers
{
    auto resolution()      const noexcept -> Extent2I { return _resolution; }
    auto noisy_texture()   const noexcept -> RawTexture2D<GLConst> { return _noisy;   }
    auto blurred_texture() const noexcept -> RawTexture2D<GLConst> { return _blurred; }

    static constexpr auto iformat = InternalFormat::R8;
    static constexpr u32  slot    = 0;

    Extent2I          _resolution = { 0, 0 };
    UniqueTexture2D   _noisy;
    UniqueTexture2D   _blurred;
    UniqueFramebuffer _fbo_noisy; // Too lazy to reuse one FBO.
    UniqueFramebuffer _fbo_blurred;
    void _resize(Extent2I new_resolution);
};

inline void AOBuffers::_resize(Extent2I new_resolution)
{
    if (_resolution == new_resolution) return;
    _resolution = new_resolution;
    _noisy   = {};
    _blurred = {};
    _noisy  ->allocate_storage(_resolution, iformat);
    _blurred->allocate_storage(_resolution, iformat);
    _fbo_noisy  ->attach_texture_to_color_buffer(_noisy,   slot);
    _fbo_blurred->attach_texture_to_color_buffer(_blurred, slot);
}


enum class SSAONoiseMode
{
    SampledFromTexture,
    GeneratedInShader,
};
JOSH3D_DEFINE_ENUM_EXTRAS(SSAONoiseMode, GeneratedInShader, SampledFromTexture);

struct SSAO
{
    bool  enable_sampling = true;   // Switch to false if you want to skip the whole stage.
    float radius = 0.2f;            // In world units.
    float bias   = 0.01f;           // Can't wait to do another reciever plane algo...
    float resolution_divisor = 2.f; // Used to scale the Occlusion buffer resolution compared to screen size.
    SSAONoiseMode noise_mode = SSAONoiseMode::GeneratedInShader;

    auto kernel_size() const noexcept -> usize { return kernel_.num_staged(); }
    // Minimum allowed angle between the surface and each kernel vector.
    auto deflection_rad() const noexcept -> float { return deflection_rad_; }
    void regenerate_kernel(usize kernel_size, float deflection_rad);

    auto noise_texture_resolution() -> Extent2I { return noise_texture_->get_resolution(); }
    void regenerate_noise_texture(Extent2I resolution);

    SSAO(
        usize    kernel_size = 12,
        float    deflection_rad = glm::radians(5.0f),
        Extent2I noise_texture_resolution = { 16, 16 });

    void operator()(RenderEnginePrimaryInterface& engine);

    AOBuffers aobuffers;

private:
    UniqueTexture2D noise_texture_;

    // NOTE: We use vec4 to avoid issues with alignment in std430,
    // even though we only need vec3 of data.
    UploadBuffer<vec4> kernel_;
    float              deflection_rad_ = {};

    UniqueSampler target_sampler_ = eval%[]{
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    };

    ShaderToken sp_sampling_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ssao_sampling.frag")});

    ShaderToken sp_blur_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ssao_blur.frag")});
};


} // namespace josh
