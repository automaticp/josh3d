#pragma once
#include "stages/primary/SSAO.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "EnumUtils.hpp"
#include "VPath.hpp"


namespace josh {


struct SSAODebug
{
    enum class OverlayMode : i32
    {
        None    = 0,
        Noisy   = 1,
        Blurred = 2,
    };

    OverlayMode mode = OverlayMode::None;

    void operator()(RenderEngineOverlayInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ovl_ssao_debug.frag")});
};
JOSH3D_DEFINE_ENUM_EXTRAS(SSAODebug::OverlayMode, None, Noisy, Blurred);


inline void SSAODebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    if (mode == OverlayMode::None) return;

    const auto* aobuffers = engine.belt().try_get<AOBuffers>();
    if (not aobuffers) return;

    const auto sp = sp_.get();

    aobuffers->noisy_texture()  .bind_to_texture_unit(0);
    aobuffers->blurred_texture().bind_to_texture_unit(1);

    sp.uniform("mode",                to_underlying(mode));
    sp.uniform("tex_noisy_occlusion", 0);
    sp.uniform("tex_occlusion",       1);

    const BindGuard bsp = sp.use();

    engine.draw_fullscreen_quad(bsp);
}


} // namespace josh::stages::overlay
