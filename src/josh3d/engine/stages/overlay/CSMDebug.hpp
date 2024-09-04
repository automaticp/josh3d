#pragma once
#include "Components.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "Transform.hpp"
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
        SharedStorageView<CascadedShadowMaps> maps)
        : gbuffer_{ std::move(gbuffer) }
        , views_  { std::move(views)   }
        , maps_   { std::move(maps)    }
    {}

    void operator()(RenderEngineOverlayInterface& engine);



private:
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

    UniqueSampler maps_sampler_ = [] {
        UniqueSampler s;
        s->set_compare_ref_depth_to_texture(false);
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        return s;
    }();

    SharedStorageView<GBuffer>            gbuffer_;
    SharedStorageView<CascadeViews>       views_;
    SharedStorageView<CascadedShadowMaps> maps_;

    UniqueBuffer<CascadeParamsGPU> csm_params_buf_;


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
    const auto& registry   = engine.registry();
    const entt::const_handle dlight_handle = get_active_directional_light(registry);

    if (dlight_handle.valid() && has_component<Transform>(dlight_handle)) {
        const glm::vec3 light_dir = dlight_handle.get<Transform>().orientation() * glm::vec3{ 0.f, 0.f, -1.f };

        BindGuard bound_camera = engine.bind_camera_ubo();

        // FIXME: Figure out what information we need here exactly.
        // Cause I'm passing data that's largely irrelevant for the debug.
        // AFAIK we only need the projview matrices.
        resize_to_fit(csm_params_buf_, NumElems{ maps_->params.size() });
        csm_params_buf_->upload_data(maps_->params);
        csm_params_buf_->bind_to_index<BufferTargetIndexed::ShaderStorage>(3);

        gbuffer_->depth_texture()  .bind_to_texture_unit(0);
        gbuffer_->normals_texture().bind_to_texture_unit(1);

        sp_views_->uniform("tex_depth",   0);
        sp_views_->uniform("tex_normals", 1);

        sp_views_->uniform("dir_light.color",     dlight_handle.get<DirectionalLight>().color);
        sp_views_->uniform("dir_light.direction", light_dir);

        glapi::disable(Capability::DepthTesting);
        {
            BindGuard bound_program = sp_views_->use();
            engine.draw_fullscreen_quad(bound_program);
        }
        glapi::enable(Capability::DepthTesting);

    }
}




inline void CSMDebug::draw_maps_overlay(
    RenderEngineOverlayInterface& engine)
{

    maps_->dir_shadow_maps_tgt.depth_attachment().texture().bind_to_texture_unit(0);
    BindGuard bound_maps_sampler =           maps_sampler_->bind_to_texture_unit(0);

    sp_maps_->uniform("cascades",   0);
    sp_maps_->uniform("cascade_id", cascade_id);

    glapi::disable(Capability::DepthTesting);
    {
        BindGuard bound_program = sp_maps_->use();
        engine.draw_fullscreen_quad(bound_program);
    }
    glapi::enable(Capability::DepthTesting);
}


} // namespace josh::stages::overlay
