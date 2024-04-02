#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
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
    dsa::UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_gamma.frag"))
            .get()
    };
};




inline void GammaCorrection::operator()(
    RenderEnginePostprocessInterface& engine)
{

    engine.screen_color().bind_to_texture_unit(0);
    sp_->uniform("color", 0);

    auto bound_program = sp_->use();

    if (use_srgb) {

        glapi::enable(Capability::SRGBConversion);
        engine.draw(bound_program);
        glapi::disable(Capability::SRGBConversion);

    } else /* custom gamma */ {

        sp_->uniform("gamma", gamma);
        engine.draw(bound_program);

    }

    bound_program.unbind();

}




} // namespace josh::stages::postprocess
