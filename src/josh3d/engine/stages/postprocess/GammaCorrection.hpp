#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderPool.hpp"
#include "VPath.hpp"
#include <entt/fwd.hpp>
#include <glbinding/gl/gl.h>




namespace josh::stages::postprocess {


class GammaCorrection {
public:
    float gamma{ 2.2f };
    bool use_srgb{ true };

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

    BindGuard bound_program = sp.use();

    if (use_srgb) {

        glapi::enable(Capability::SRGBConversion);
        engine.draw(bound_program);
        glapi::disable(Capability::SRGBConversion);

    } else /* custom gamma */ {

        sp.uniform("gamma", gamma);
        engine.draw(bound_program);

    }

}




} // namespace josh::stages::postprocess
