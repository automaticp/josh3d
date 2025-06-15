#pragma once
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"


namespace josh {


struct HDR
{
    bool  use_reinhard = false;
    bool  use_exposure = true;
    float exposure     = 1.0f;

    void operator()(RenderEnginePostprocessInterface& engine);

private:
    ShaderToken sp_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/pp_hdr.frag")});
};


inline void HDR::operator()(
    RenderEnginePostprocessInterface& engine)
{
    const auto sp = sp_.get();
    engine.screen_color().bind_to_texture_unit(0);

    sp.uniform("color",        0);
    sp.uniform("use_reinhard", use_reinhard);
    sp.uniform("use_exposure", use_exposure);
    sp.uniform("exposure",     exposure);

    const BindGuard bsp = sp.use();

    engine.draw(bsp);
}


} // namespace josh
