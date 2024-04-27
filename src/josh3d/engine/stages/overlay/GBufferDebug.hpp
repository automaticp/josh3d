#pragma once
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
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
public:
    enum class OverlayMode : GLint {
        None         = 0,
        Albedo       = 1,
        Specular     = 2,
        Position     = 3,
        Depth        = 4,
        DepthLinear  = 5,
        Normals      = 6,
        DrawRegion   = 7,
        ObjectID     = 8,
    };

    OverlayMode mode{ OverlayMode::None };

    GBufferDebug(SharedStorageView<GBuffer> gbuffer)
        : gbuffer_{ std::move(gbuffer) }
    {}

    void operator()(RenderEngineOverlayInterface& engine);



private:
    SharedStorageView<GBuffer> gbuffer_;

    UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ovl_gbuffer_debug.frag"))
            .get()
    };

    UniqueSampler integer_sampler_ = [] {
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        return s;
    }();



};




inline void GBufferDebug::operator()(
    RenderEngineOverlayInterface& engine)
{

    if (mode == OverlayMode::None) {
        return;
    }

    gbuffer_->position_draw_texture().bind_to_texture_unit(0);
    gbuffer_->normals_texture()      .bind_to_texture_unit(1);
    gbuffer_->albedo_spec_texture()  .bind_to_texture_unit(2);
    gbuffer_->depth_texture()        .bind_to_texture_unit(3);
    gbuffer_->object_id_texture()    .bind_to_texture_unit(4);
    integer_sampler_->bind_to_texture_unit(4);

    sp_->uniform("mode",              to_underlying(mode));
    sp_->uniform("z_near",            engine.camera().get_params().z_near);
    sp_->uniform("z_far",             engine.camera().get_params().z_far);
    sp_->uniform("tex_position_draw", 0);
    sp_->uniform("tex_normals",       1);
    sp_->uniform("tex_albedo_spec",   2);
    sp_->uniform("tex_depth",         3);
    sp_->uniform("tex_object_id",     4);


    engine.draw_fullscreen_quad(sp_->use());


    unbind_sampler_from_unit(4);
}




} // namespace josh::stages::overlay

