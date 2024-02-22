#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>




namespace josh::stages::postprocess {


class FXAA {
private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_fxaa.frag"))
            .get()
    };

public:
    bool use_fxaa{ true };

    float gamma{ 2.2f };
    float absolute_contrast_threshold = 0.0312f; // gamma-dependant
    float relative_contrast_threshold = 0.125f;

    void operator()(RenderEnginePostprocessInterface& engine);

};




inline void FXAA::operator()(
    RenderEnginePostprocessInterface& engine)
{
    if (!use_fxaa) { return; }


    engine.screen_color().bind_to_unit_index(0);

    sp_.use()
        .uniform("color", 0)
        .uniform("gamma", gamma)
        .uniform("absolute_contrast_threshold", absolute_contrast_threshold)
        .uniform("relative_contrast_threshold", relative_contrast_threshold)
        .and_then([&] {
            engine.draw();
        });

}




} // namespace josh::stages::postprocess
