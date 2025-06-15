#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"
#include <entt/fwd.hpp>
#include <glbinding/gl/gl.h>




namespace josh {

/*
HMM: I don't think this is used anywhere?
We just use FRAMEBUFFER_SRGB instead, no?
*/
struct GammaCorrection
{
    bool  use_srgb = true;
    float gamma    = 2.2f;

    void operator()(RenderEnginePostprocessInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_gamma.frag")});
};


inline void GammaCorrection::operator()(
    RenderEnginePostprocessInterface& engine)
{
    const auto sp = sp_.get();
    engine.screen_color().bind_to_texture_unit(0);
    sp.uniform("color", 0);

    const BindGuard bsp = sp.use();

    if (use_srgb)
    {
        glapi::enable(Capability::SRGBConversion);
        engine.draw(bsp);
        glapi::disable(Capability::SRGBConversion);
    }
    else /* custom gamma */
    {
        sp.uniform("gamma", gamma);
        engine.draw(bsp);
    }
}




} // namespace josh::stages::postprocess
