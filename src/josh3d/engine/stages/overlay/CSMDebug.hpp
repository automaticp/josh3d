#pragma once
#include "EnumUtils.hpp"
#include "GLAPICommonTypes.hpp"
#include "Scalars.hpp"
#include "UniformTraits.hpp"
#include "GLObjects.hpp"
#include "UploadBuffer.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "StageContext.hpp"
#include "VPath.hpp"


namespace josh {


struct CSMDebug
{
    enum class OverlayMode
    {
        None,
        Views,
        Maps,
    };

    OverlayMode mode = OverlayMode::None;

    // NOTE: The follwing are mere hints because the real number of
    // cascades might have changed before you selected one, and the frame
    // actually updated the cascades. This works OK most of the time still.

    auto num_cascades_hint() const noexcept -> usize { return last_num_cascades_; }
    auto current_cascade_idx() const noexcept -> uindex { return last_cascade_idx_; }
    void select_cascade(uindex desired_cascade_idx) { desired_cascade_idx_ = desired_cascade_idx; }

    void operator()(OverlayContext context);

private:
    void draw_views_overlay(OverlayContext context);
    void draw_maps_overlay (OverlayContext context);

    // What a pain...
    uindex desired_cascade_idx_ = 0;
    uindex last_cascade_idx_    = 0;
    usize  last_num_cascades_   = 1;
    void _update_cascade_info(const Cascades& cascades);

    UploadBuffer<CascadeViewGPU> csm_views_buf_;

    UniqueSampler maps_sampler_ = []{
        UniqueSampler s;
        s->set_compare_ref_depth_to_texture(false);
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        return s;
    }();

    ShaderToken sp_views_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_csm_debug_views.frag")});

    ShaderToken sp_maps_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_csm_debug_maps.frag")});
};
JOSH3D_DEFINE_ENUM_EXTRAS(CSMDebug::OverlayMode, None, Views, Maps);


} // namespace josh
