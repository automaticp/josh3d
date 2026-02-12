#pragma once
#include "GLObjects.hpp"
#include "StageContext.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"
#include "Tracy.hpp"


namespace josh {


struct FXAA
{
    bool  use_fxaa   = true;
    u32   debug_mode = 0;
    u32   luma_mode  = 0;
    float gamma      = 2.2f;
    float absolute_contrast_threshold = 0.0312f; // Gamma-dependent.
    float relative_contrast_threshold = 0.125f;
    float pixel_blend_strength        = 1.0f;  // [0, 1]
    float gradient_threshold_fraction = 0.25f;
    u32   stride_table_idx            = 2;
    float guess_jump                  = 8.0;

    void operator()(PostprocessContext context);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/screen_quad.vert"),
        .frag = VPath("src/shaders/pp_fxaa.frag")});
};


inline void FXAA::operator()(
    PostprocessContext context)
{
    ZSCGPUN("FXAA");
    if (not use_fxaa) return;

    const auto sp = sp_.get();

    context.main_front_color_texture().bind_to_texture_unit(0);

    sp.uniform("color",      0);
    sp.uniform("debug_mode", debug_mode);
    sp.uniform("luma_mode",  luma_mode);
    sp.uniform("gamma",      gamma);
    sp.uniform("absolute_contrast_threshold", absolute_contrast_threshold);
    sp.uniform("relative_contrast_threshold", relative_contrast_threshold);
    sp.uniform("pixel_blend_strength",        pixel_blend_strength);
    sp.uniform("gradient_threshold_fraction", gradient_threshold_fraction);
    sp.uniform("stride_table_idx",            stride_table_idx);
    sp.uniform("guess_jump",                  guess_jump);

    const BindGuard bsp = sp.use();

    context.draw_quad_and_swap(bsp);
}


} // namespace josh
