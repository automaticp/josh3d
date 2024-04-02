#pragma once
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "LightCasters.hpp"
#include "stages/precompute/CSMSetup.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include "stages/primary/GBufferStorage.hpp"


namespace josh::stages::overlay {


class CSMDebug {
public:
    enum class OverlayMode : GLint {
        None  = 0,
        Views = 1,
        Maps  = 2,
    } mode { OverlayMode::None };

    GLuint num_cascades() const noexcept { return maps_->params.size(); }
    GLuint cascade_id{ 0 };

    CSMDebug(
        SharedStorageView<GBuffer>            gbuffer,
        SharedStorageView<CascadeViews>       views,
        SharedStorageView<CascadedShadowMaps> maps
    )
        : gbuffer_{ std::move(gbuffer) }
        , views_  { std::move(views)   }
        , maps_   { std::move(maps)    }
    {}

    void operator()(RenderEngineOverlayInterface& engine);



private:
    dsa::UniqueProgram sp_views_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_csm_debug_views.frag"))
            .get()
    };

    dsa::UniqueProgram sp_maps_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_csm_debug_maps.frag"))
            .get()
    };

    dsa::UniqueSampler maps_sampler_ = [] {
        dsa::UniqueSampler s;
        s->set_compare_ref_depth_to_texture(false);
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        return s;
    }();

    SharedStorageView<GBuffer>            gbuffer_;
    SharedStorageView<CascadeViews>       views_;
    SharedStorageView<CascadedShadowMaps> maps_;

    dsa::UniqueBuffer<CascadeParams> csm_params_buf_;


    void draw_views_overlay(RenderEngineOverlayInterface& engine);
    void draw_maps_overlay(RenderEngineOverlayInterface& engine);

};




inline void CSMDebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    switch (mode) {
        case OverlayMode::None:  return;
        case OverlayMode::Views: return draw_views_overlay(engine);
        case OverlayMode::Maps:  return draw_maps_overlay(engine);
    }
}




inline void CSMDebug::draw_views_overlay(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();

    // FIXME: Figure out what information we need here exactly.
    // Cause I'm passing data that's largely irrelevant for the debug.
    // AFAIK we only need the projview matrices.
    dsa::resize_to_fit(csm_params_buf_, NumElems{ maps_->params.size() });
    csm_params_buf_->upload_data(maps_->params);
    csm_params_buf_->bind_to_index<BufferTargetIndexed::ShaderStorage>(3);

    gbuffer_->position_draw_texture().bind_to_texture_unit(0);
    gbuffer_->normals_texture()      .bind_to_texture_unit(1);

    sp_views_->uniform("tex_position_draw", 0);
    sp_views_->uniform("tex_normals",       1);

    const auto& storage = registry.storage<light::Directional>();

    if (auto dir_light_it = storage.begin(); dir_light_it != storage.end()) {

        sp_views_->uniform("dir_light.color",     dir_light_it->color);
        sp_views_->uniform("dir_light.direction", dir_light_it->direction);

        glapi::disable(Capability::DepthTesting);
        {
            auto bound_program = sp_views_->use();
            engine.draw_fullscreen_quad(bound_program);
            bound_program.unbind();
        }
        glapi::enable(Capability::DepthTesting);

    }

}




inline void CSMDebug::draw_maps_overlay(
    RenderEngineOverlayInterface& engine)
{

    maps_->dir_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(0);
    maps_sampler_->bind_to_texture_unit(0);

    sp_maps_->uniform("cascades",   0);
    sp_maps_->uniform("cascade_id", cascade_id);

    glapi::disable(Capability::DepthTesting);
    {
        auto bound_program = sp_maps_->use();
        engine.draw_fullscreen_quad(bound_program);
        bound_program.unbind();
    }
    glapi::enable(Capability::DepthTesting);

    unbind_sampler_from_unit(0);
}


} // namespace josh::stages::overlay
