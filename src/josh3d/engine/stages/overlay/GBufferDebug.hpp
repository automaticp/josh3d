#pragma once
#include "stages/primary/GBufferStorage.hpp"
#include "EnumUtils.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "GLScalars.hpp"
#include "VPath.hpp"
#include <glbinding/gl/gl.h>
#include <entt/entity/fwd.hpp>




namespace josh::stages::overlay {


class GBufferDebug {
private:
    SharedStorageView<GBuffer> gbuffer_;

    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_gbuffer_debug.frag"))
            .get()
    };

public:
    enum class OverlayMode : GLint {
        none         = 0,
        albedo       = 1,
        specular     = 2,
        position     = 3,
        depth        = 4,
        depth_linear = 5,
        normals      = 6,
        draw_region  = 7,
        object_id    = 8,
    };

    OverlayMode mode{ OverlayMode::none };

    GBufferDebug(SharedStorageView<GBuffer> gbuffer)
        : gbuffer_{ std::move(gbuffer) }
    {}

    void operator()(RenderEngineOverlayInterface& engine);

};




inline void GBufferDebug::operator()(
    RenderEngineOverlayInterface& engine)
{

    if (mode == OverlayMode::none) {
        return;
    }

    gbuffer_->position_draw_texture().bind_to_unit_index(0);
    gbuffer_->normals_texture()      .bind_to_unit_index(1);
    gbuffer_->albedo_spec_texture()  .bind_to_unit_index(2);
    gbuffer_->depth_texture()        .bind_to_unit_index(3);
    gbuffer_->object_id_texture()    .bind_to_unit_index(4);
    // engine   .screen_depth()         .bind_to_unit_index(3);

    sp_.use()
        .uniform("mode",              to_underlying(mode))
        .uniform("z_near",            engine.camera().get_params().z_near)
        .uniform("z_far",             engine.camera().get_params().z_far)
        .uniform("tex_position_draw", 0)
        .uniform("tex_normals",       1)
        .uniform("tex_albedo_spec",   2)
        .uniform("tex_depth",         3)
        .uniform("tex_object_id",     4)
        .and_then([&] {
            engine.draw_fullscreen_quad();
        });

}




} // namespace josh::stages::overlay

