#pragma once
#include "EnumUtils.hpp"
#include "GBuffer.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "GLScalars.hpp"
#include "VPath.hpp"
#include <glbinding/gl/gl.h>
#include <entt/entity/fwd.hpp>




namespace josh {

class PostprocessGBufferDebugOverlayStage {
private:
    SharedStorageView<GBuffer> gbuffer_;

    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_gbuffer_debug.frag"))
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
    };

    OverlayMode mode{ OverlayMode::none };

    PostprocessGBufferDebugOverlayStage(SharedStorageView<GBuffer> gbuffer)
        : gbuffer_{ std::move(gbuffer) }
    {}

    void operator()(const RenderEnginePostprocessInterface& engine, const entt::registry&) {

        if (mode == OverlayMode::none) {
            return;
        }

        gbuffer_->position_target()   .bind_to_unit_index(0);
        gbuffer_->normals_target()    .bind_to_unit_index(1);
        gbuffer_->albedo_spec_target().bind_to_unit_index(2);
        engine   .screen_depth()      .bind_to_unit_index(3);

        sp_.use()
            .uniform("mode",              to_underlying(mode))
            .uniform("z_near",            engine.camera().get_params().z_near)
            .uniform("z_far",             engine.camera().get_params().z_far)
            .uniform("tex_position_draw", 0)
            .uniform("tex_normals",       1)
            .uniform("tex_albedo_spec",   2)
            .uniform("tex_depth",         3)
            .and_then([&] {
                engine.draw_to_front();
            });

    }

};


} // namespace josh
