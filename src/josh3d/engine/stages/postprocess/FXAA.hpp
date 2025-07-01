#pragma once
#include "GLObjects.hpp"
#include "StageContext.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"
#include "Tracy.hpp"


namespace josh {


struct FXAA
{
    bool use_fxaa = true;

    float gamma = 2.2f;
    float absolute_contrast_threshold = 0.0312f; // gamma-dependant
    float relative_contrast_threshold = 0.125f;

    void operator()(PostprocessContext context);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_fxaa.frag")});
};


inline void FXAA::operator()(
    PostprocessContext context)
{
    ZSCGPUN("FXAA");
    if (not use_fxaa) return;

    const auto sp = sp_.get();

    context.main_front_color_texture().bind_to_texture_unit(0);

    sp.uniform("color", 0);
    sp.uniform("gamma", gamma);
    sp.uniform("absolute_contrast_threshold", absolute_contrast_threshold);
    sp.uniform("relative_contrast_threshold", relative_contrast_threshold);

    const BindGuard bsp = sp.use();

    context.draw_quad_and_swap(bsp);
}


} // namespace josh
