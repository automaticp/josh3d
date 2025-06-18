#pragma once
#include "GLObjectHelpers.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "LightsGPU.hpp"
#include "ShaderPool.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"


namespace josh {


struct DeferredShading
{
    enum class Mode
    {
        SinglePass,
        MultiPass
    };

    struct PointShadowParams
    {
        vec2  bias_bounds = { 0.0001f, 0.08f };
        i32   pcf_extent  = 1;
        float pcf_offset  = 0.01f;
    };

    struct DirShadowParams
    {
        float base_bias_tx = 0.2f;
        i32   pcf_extent   = 1;
        float pcf_offset   = 1.0f;
    };

    Mode mode = Mode::SinglePass;

    PointShadowParams point_params;
    DirShadowParams   dir_params;

    bool  use_ambient_occlusion   = true;
    float ambient_occlusion_power = 0.8f;

    float plight_fade_start_fraction = 0.75f; // [0, 1] in fraction of bounding radius.

    void operator()(RenderEnginePrimaryInterface& engine);

private:
    UploadBuffer<PointLightBoundedGPU> plights_with_shadow_buf_;
    UploadBuffer<PointLightBoundedGPU> plights_no_shadow_buf_;
    UploadBuffer<CascadeViewGPU>       csm_views_buf_;

    void update_point_light_buffers(const Registry& registry);
    void update_cascade_buffer(const Cascades& csm);

    void draw_singlepass(RenderEnginePrimaryInterface& engine);
    void draw_multipass (RenderEnginePrimaryInterface& engine);

        UniqueSampler _target_sampler = create_sampler({
        .min_filter = MinFilter::Nearest,
        .mag_filter = MagFilter::Nearest,
        .wrap_all   = Wrap::ClampToEdge,
    });

    UniqueSampler _ao_sampler = create_sampler({
        .min_filter = MinFilter::Linear,
        .mag_filter = MagFilter::Linear,
        .wrap_all   = Wrap::ClampToEdge,
    });

    UniqueSampler _csm_sampler = create_sampler({
        .min_filter   = MinFilter::Linear,
        .mag_filter   = MagFilter::Linear,
        .wrap_all     = Wrap::ClampToBorder,
        .border_color = { .r=1.f },
         // Enable shadow sampling with built-in 2x2 PCF
        .compare_ref_depth_to_texture = true,
        // Comparison: result = ref OPERATOR texture
        // This will return "how much this fragment is lit" from 0 to 1.
        // If you want "how much it's in shadow", use (1.0 - result).
        // Or set the comparison func to Greater.
        .compare_func = CompareOp::Less,
    });

    UniqueSampler _psm_sampler = create_sampler({
        .min_filter   = MinFilter::Linear,
        .mag_filter   = MagFilter::Linear,
        .wrap_all     = Wrap::ClampToEdge,
        .compare_ref_depth_to_texture = true,
        .compare_func = CompareOp::Less,
    });

    ShaderToken sp_singlepass_ = shader_pool().get({
        .vert = VPath("src/shaders/dfr_shading.vert"),
        .frag = VPath("src/shaders/dfr_shading_singlepass.frag")});

    ShaderToken sp_pass_plight_with_shadow_ = shader_pool().get({
        .vert = VPath("src/shaders/dfr_shading_point.vert"),
        .frag = VPath("src/shaders/dfr_shading_point_with_shadow.frag")});

    ShaderToken sp_pass_plight_no_shadow_ = shader_pool().get({
        .vert = VPath("src/shaders/dfr_shading_point.vert"),
        .frag = VPath("src/shaders/dfr_shading_point_no_shadow.frag")});

    ShaderToken sp_pass_ambi_dir_ = shader_pool().get({
        .vert = VPath("src/shaders/dfr_shading.vert"),
        .frag = VPath("src/shaders/dfr_shading_ambi_dir.frag")});
};
JOSH3D_DEFINE_ENUM_EXTRAS(DeferredShading::Mode, SinglePass, MultiPass);


} // namespace josh
