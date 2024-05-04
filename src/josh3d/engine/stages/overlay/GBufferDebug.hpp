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

    BindGuard bound_camera = engine.bind_camera_ubo();

    gbuffer_->depth_texture()    .bind_to_texture_unit(0);
    gbuffer_->normals_texture()  .bind_to_texture_unit(1);
    gbuffer_->albedo_texture()   .bind_to_texture_unit(2);
    gbuffer_->specular_texture() .bind_to_texture_unit(3);
    gbuffer_->object_id_texture().bind_to_texture_unit(4);

    BindGuard bound_integer_sampler = integer_sampler_->bind_to_texture_unit(4);

    sp_->uniform("mode",                 to_underlying(mode));
    sp_->uniform("gbuffer.tex_depth",    0);
    sp_->uniform("gbuffer.tex_normals",  1);
    sp_->uniform("gbuffer.tex_albedo",   2);
    sp_->uniform("gbuffer.tex_specular", 3);
    sp_->uniform("tex_object_id",        4);


    engine.draw_fullscreen_quad(sp_->use());

}




} // namespace josh::stages::overlay

