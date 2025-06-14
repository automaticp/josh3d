#pragma once
#include "GLAPICommonTypes.hpp"
#include "UniformTraits.hpp"
#include "GLObjects.hpp"
#include "UploadBuffer.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "RenderEngine.hpp"
#include "VPath.hpp"


namespace josh::stages::overlay {


class CSMDebug {
public:
    using Cascades       = primary::Cascades;
    using CascadeView    = primary::CascadeView;
    using CascadeViewGPU = primary::CascadeViewGPU;

    enum class OverlayMode : GLint {
        None  = 0,
        Views = 1,
        Maps  = 2,
    } mode { OverlayMode::None };

    // TODO: Hmm, interesting... This doesn't work.
    // We could query the pipeline directly though.
    auto num_cascades() const noexcept { return last_num_cascades_; }
    GLuint cascade_id{ 0 };

    void operator()(RenderEngineOverlayInterface& engine);

private:
    usize last_num_cascades_;

    ShaderToken sp_views_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_csm_debug_views.frag")});

    ShaderToken sp_maps_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_csm_debug_maps.frag")});

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
