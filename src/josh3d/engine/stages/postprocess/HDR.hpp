#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>




namespace josh::stages::postprocess {


class HDR {
public:
    bool use_reinhard{ false };
    bool use_exposure{ true };
    float exposure{ 1.0f };

    void operator()(RenderEnginePostprocessInterface& engine);


private:
    UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_hdr.frag"))
            .get()
    };

};




inline void HDR::operator()(
    RenderEnginePostprocessInterface& engine)
{

    engine.screen_color().bind_to_texture_unit(0);
    sp_->uniform("color",        0);
    sp_->uniform("use_reinhard", use_reinhard);
    sp_->uniform("use_exposure", use_exposure);
    sp_->uniform("exposure",     exposure);

    auto bound_program = sp_->use();
    engine.draw(bound_program);
    bound_program.unbind();

}




} // namespace josh::stages::postprocess
