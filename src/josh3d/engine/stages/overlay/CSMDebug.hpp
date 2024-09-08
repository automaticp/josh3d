#pragma once
#include "GLAPICommonTypes.hpp"
#include "UniformTraits.hpp"
#include "GLObjects.hpp"
#include "UploadBuffer.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include "stages/primary/GBufferStorage.hpp"


namespace josh::stages::overlay {


class CSMDebug {
public:
    using Cascades       = primary::CascadedShadowMapping::Cascades;
    using CascadeView    = primary::CascadedShadowMapping::CascadeView;
    using CascadeViewGPU = primary::CascadedShadowMapping::CascadeViewGPU;

    enum class OverlayMode : GLint {
        None  = 0,
        Views = 1,
        Maps  = 2,
    } mode { OverlayMode::None };

    auto num_cascades() const noexcept { return cascades_->views.size(); }
    GLuint cascade_id{ 0 };

    CSMDebug(
        SharedView<GBuffer>  input_gbuffer,
        SharedView<Cascades> input_cascades
    )
        : gbuffer_ { std::move(input_gbuffer)  }
        , cascades_{ std::move(input_cascades) }
    {}

    void operator()(RenderEngineOverlayInterface& engine);

private:
    SharedView<GBuffer>  gbuffer_;
    SharedView<Cascades> cascades_;

    UniqueProgram sp_views_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_csm_debug_views.frag"))
            .get()
    };

    UniqueProgram sp_maps_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_csm_debug_maps.frag"))
            .get()
    };

    UniqueSampler maps_sampler_ = []{
        UniqueSampler s;
        s->set_compare_ref_depth_to_texture(false);
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        return s;
    }();

    UploadBuffer<CascadeViewGPU> csm_views_buf_;


    void draw_views_overlay(RenderEngineOverlayInterface& engine);
    void draw_maps_overlay (RenderEngineOverlayInterface& engine);

};




} // namespace josh::stages::overlay
