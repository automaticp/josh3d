#pragma once
#include "GLObjects.hpp"
#include "StageContext.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"
#include "Tracy.hpp"
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

    void operator()(PostprocessContext context);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_gamma.frag")});
};


inline void GammaCorrection::operator()(
    PostprocessContext context)
{
    ZSCGPUN("GammaCorrection");
    const auto sp = sp_.get();
    context.main_front_color_texture().bind_to_texture_unit(0);
    sp.uniform("color", 0);

    const BindGuard bsp = sp.use();

    if (use_srgb)
    {
        glapi::enable(Capability::SRGBConversion);
        context.draw_quad_and_swap(bsp);
        glapi::disable(Capability::SRGBConversion);
    }
    else /* custom gamma */
    {
        sp.uniform("gamma", gamma);
        context.draw_quad_and_swap(bsp);
    }
}




} // namespace josh::stages::postprocess
