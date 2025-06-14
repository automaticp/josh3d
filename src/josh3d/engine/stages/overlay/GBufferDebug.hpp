#pragma once
#include "GLAPIBinding.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjects.hpp"
#include "ShaderPool.hpp"
#include "EnumUtils.hpp"
#include "RenderEngine.hpp"
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

    void operator()(RenderEngineOverlayInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_gbuffer_debug.frag")});

    UniqueSampler integer_sampler_ = []{
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        return s;
    }();

};




inline void GBufferDebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    if (mode == OverlayMode::None) return;

    auto* gbuffer = engine.belt().try_get<GBuffer>();

    if (not gbuffer) return;

    BindGuard bound_camera = engine.bind_camera_ubo();

    gbuffer->depth_texture()    .bind_to_texture_unit(0);
    gbuffer->normals_texture()  .bind_to_texture_unit(1);
    gbuffer->albedo_texture()   .bind_to_texture_unit(2);
    gbuffer->specular_texture() .bind_to_texture_unit(3);
    gbuffer->object_id_texture().bind_to_texture_unit(4);

    BindGuard bound_integer_sampler = integer_sampler_->bind_to_texture_unit(4);

    const auto sp = sp_.get();

    sp.uniform("mode",                 to_underlying(mode));
    sp.uniform("gbuffer.tex_depth",    0);
    sp.uniform("gbuffer.tex_normals",  1);
    sp.uniform("gbuffer.tex_albedo",   2);
    sp.uniform("gbuffer.tex_specular", 3);
    sp.uniform("tex_object_id",        4);


    engine.draw_fullscreen_quad(sp.use());

}




} // namespace josh::stages::overlay

