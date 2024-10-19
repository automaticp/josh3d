#pragma once
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "LightsGPU.hpp"
#include "ShaderPool.hpp"
#include "SharedStorage.hpp"
#include "GBufferStorage.hpp"
#include "UploadBuffer.hpp"
#include "VPath.hpp"
#include "stages/primary/PointShadowMapping.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/SSAO.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <utility>


namespace josh::stages::primary {


class DeferredShading {
public:
    using Cascades       = CascadedShadowMapping::Cascades;
    using CascadeView    = CascadedShadowMapping::CascadeView;
    using CascadeViewGPU = CascadedShadowMapping::CascadeViewGPU;
    using PointShadows   = PointShadowMapping::PointShadows;

    enum class Mode {
        SinglePass,
        MultiPass
    };

    struct PointShadowParams {
        glm::vec2 bias_bounds{ 0.0001f, 0.08f };
        GLint     pcf_extent{ 1 };
        GLfloat   pcf_offset{ 0.01f };
    };

    struct DirShadowParams {
        GLfloat base_bias_tx{ 0.2f };
        GLint   pcf_extent{ 1 };
        GLfloat pcf_offset{ 1.0f };
    };


    Mode mode{ Mode::SinglePass };

    PointShadowParams point_params;
    DirShadowParams   dir_params;

    bool  use_ambient_occlusion  { true };
    float ambient_occlusion_power{ 0.8f };

    float plight_fade_start_fraction { 0.75f }; // [0, 1] in fraction of bounding radius.


    DeferredShading(
        SharedView<GBuffer>                 gbuffer,
        SharedView<PointShadows>            input_psm,
        SharedView<Cascades>                input_csm,
        SharedView<AmbientOcclusionBuffers> input_ao
    )
        : gbuffer_  { std::move(gbuffer)   }
        , input_psm_{ std::move(input_psm) }
        , input_csm_{ std::move(input_csm) }
        , input_ao_ { std::move(input_ao)  }
    {}

    void operator()(RenderEnginePrimaryInterface&);


private:
    SharedView<GBuffer>                 gbuffer_;
    SharedView<PointShadows>            input_psm_;
    SharedView<Cascades>                input_csm_;
    SharedView<AmbientOcclusionBuffers> input_ao_;

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

    UploadBuffer<PointLightBoundedGPU> plights_with_shadow_buf_;
    UploadBuffer<PointLightBoundedGPU> plights_no_shadow_buf_;
    UploadBuffer<CascadeViewGPU>       csm_views_buf_;

    UniqueSampler target_sampler_ = []() {
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    }();

    UniqueSampler csm_sampler_ = []() {
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_border_color_float({ .r=1.f });
        s->set_wrap_all(Wrap::ClampToBorder);
        // Enable shadow sampling with built-in 2x2 PCF
        s->set_compare_ref_depth_to_texture(true);
        // Comparison: result = ref OPERATOR texture
        // This will return "how much this fragment is lit" from 0 to 1.
        // If you want "how much it's in shadow", use (1.0 - result).
        // Or set the comparison func to Greater.
        s->set_compare_func(CompareOp::Less);
        return s;
    }();

    UniqueSampler psm_sampler_ = []() {
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_wrap_all(Wrap::ClampToEdge);
        s->set_compare_ref_depth_to_texture(true);
        s->set_compare_func(CompareOp::Less);
        return s;
    }();

    void update_point_light_buffers(const entt::registry& registry);
    void update_cascade_buffer();

    void draw_singlepass(RenderEnginePrimaryInterface& engine);
    void draw_multipass (RenderEnginePrimaryInterface& engine);

};


} // namespace josh::stages::primary
