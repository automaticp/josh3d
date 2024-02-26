#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "LightCasters.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "stages/precompute/CSMSetup.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "EnumUtils.hpp"
#include "VPath.hpp"
#include "stages/primary/GBufferStorage.hpp"


namespace josh::stages::overlay {


class CSMDebug {
private:
    UniqueShaderProgram sp_views_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_csm_debug_views.frag"))
            .get()
    };

    UniqueShaderProgram sp_maps_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_csm_debug_maps.frag"))
            .get()
    };

    UniqueSampler maps_sampler_ = [] {
        UniqueSampler s;
        s.set_compare_mode_none();
        s.set_min_mag_filters(gl::GL_NEAREST, gl::GL_NEAREST);
        return s;
    }();

    SharedStorageView<GBuffer>            gbuffer_;
    SharedStorageView<CascadeViews>       views_;
    SharedStorageView<CascadedShadowMaps> maps_;

    SSBOWithIntermediateBuffer<CascadeParams> cascade_params_ssbo_{
        3, gl::GL_DYNAMIC_DRAW
    };

public:
    enum class OverlayMode : GLint {
        none  = 0,
        views = 1,
        maps  = 2,
    } mode { OverlayMode::none };

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
    void draw_views_overlay(RenderEngineOverlayInterface& engine);
    void draw_maps_overlay(RenderEngineOverlayInterface& engine);

};




inline void CSMDebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    switch (mode) {
        case OverlayMode::none:  return;
        case OverlayMode::views: return draw_views_overlay(engine);
        case OverlayMode::maps:  return draw_maps_overlay(engine);
    }
}




inline void CSMDebug::draw_views_overlay(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();

    // FIXME: Figure out what information we need here exactly.
    // Cause I'm passing data that's largely irrelevant for the debug.
    // AFAIK we only need the projview matrices.
    cascade_params_ssbo_.bind().update(maps_->params);

    sp_views_.use().and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {

        gbuffer_->position_draw_texture().bind_to_unit_index(0);
        gbuffer_->normals_texture()      .bind_to_unit_index(1);

        ashp.uniform("tex_position_draw", 0)
            .uniform("tex_normals",       1);

        const auto& storage = registry.storage<light::Directional>();
        if (auto dir_light_it = storage.begin(); dir_light_it != storage.end()) {

            ashp.uniform("dir_light.color",     dir_light_it->color)
                .uniform("dir_light.direction", dir_light_it->direction);

            using namespace gl;

            glDisable(GL_DEPTH_TEST);
            engine.draw_fullscreen_quad();
            glEnable(GL_DEPTH_TEST);

        }

    });

}




inline void CSMDebug::draw_maps_overlay(
    RenderEngineOverlayInterface& engine)
{

    maps_->dir_shadow_maps_tgt.depth_attachment().texture().bind_to_unit_index(0);
    auto bound_sampler = maps_sampler_.bind_to_unit_index(0);

    sp_maps_.use()
        .uniform("cascades",   0)
        .uniform("cascade_id", cascade_id)
        .and_then([&] {
            using namespace gl;
            glDisable(GL_DEPTH_TEST);
            engine.draw_fullscreen_quad();
            glEnable(GL_DEPTH_TEST);
        });

    bound_sampler.unbind();
}


} // namespace josh::stages::overlay
