#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>




namespace josh::stages::postprocess {


class FXAA {
public:
    bool use_fxaa{ true };

    float gamma{ 2.2f };
    float absolute_contrast_threshold = 0.0312f; // gamma-dependant
    float relative_contrast_threshold = 0.125f;

    void operator()(RenderEnginePostprocessInterface& engine);


private:
    dsa::UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_fxaa.frag"))
            .get()
    };

};




inline void FXAA::operator()(
    RenderEnginePostprocessInterface& engine)
{
    if (!use_fxaa) { return; }


    engine.screen_color().bind_to_texture_unit(0);


    sp_->uniform("color", 0);
    sp_->uniform("gamma", gamma);
    sp_->uniform("absolute_contrast_threshold", absolute_contrast_threshold);
    sp_->uniform("relative_contrast_threshold", relative_contrast_threshold);

    auto bound_program = sp_->use();
    engine.draw(bound_program);
    bound_program.unbind();

}




} // namespace josh::stages::postprocess
