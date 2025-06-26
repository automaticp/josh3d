#pragma once
#include "stages/primary/SSAO.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "EnumUtils.hpp"
#include "VPath.hpp"
#include "Tracy.hpp"


namespace josh {


struct SSAODebug
{
    enum class OverlayMode : i32
    {
        None,
        Backbuffer,
        Occlusion,
    };

    OverlayMode mode = OverlayMode::None;

    void operator()(RenderEngineOverlayInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/screen_quad.vert"),
        .frag = VPath("src/shaders/ovl_ssao_debug.frag")});
};
JOSH3D_DEFINE_ENUM_EXTRAS(SSAODebug::OverlayMode, None, Backbuffer, Occlusion);


inline void SSAODebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    ZSCGPUN("SSAODebug");
    if (mode == OverlayMode::None) return;

    const auto* aobuffers = engine.belt().try_get<AOBuffers>();
    if (not aobuffers) return;

    const auto sp = sp_.get();

    aobuffers->backbuffer_texture().bind_to_texture_unit(0);
    aobuffers->occlusion_texture() .bind_to_texture_unit(1);

    sp.uniform("mode",           to_underlying(mode));
    sp.uniform("tex_backbuffer", 0);
    sp.uniform("tex_occlusion",  1);

    const BindGuard bsp = sp.use();

    engine.draw_fullscreen_quad(bsp);
}


} // namespace josh::stages::overlay
