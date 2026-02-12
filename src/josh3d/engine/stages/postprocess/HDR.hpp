#pragma once
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "StageContext.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"
#include "Tracy.hpp"


namespace josh {


struct HDR
{
    bool  use_reinhard = false;
    bool  use_exposure = true;
    float exposure     = 1.0f;

    void operator()(PostprocessContext context);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_hdr.frag")});
};


inline void HDR::operator()(
    PostprocessContext context)
{
    ZSCGPUN("HDR");
    const auto sp = sp_.get();
    context.main_front_color_texture().bind_to_texture_unit(0);

    sp.uniform("color",        0);
    sp.uniform("use_reinhard", use_reinhard);
    sp.uniform("use_exposure", use_exposure);
    sp.uniform("exposure",     exposure);

    const BindGuard bsp = sp.use();

    context.draw_quad_and_swap(bsp);
}


} // namespace josh
