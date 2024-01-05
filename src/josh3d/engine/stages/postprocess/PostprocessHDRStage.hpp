#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>



namespace josh {

class PostprocessHDRStage {
private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_hdr.frag"))
            .get()
    };

public:
    bool use_reinhard{ false };
    bool use_exposure{ true };
    float exposure{ 1.0f };

    void operator()(const RenderEnginePostprocessInterface& engine, const entt::registry&) {
        using namespace gl;

        sp_.use().and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            ashp.uniform("color", 0)
                .uniform("use_reinhard", use_reinhard)
                .uniform("use_exposure", use_exposure)
                .uniform("exposure", exposure);

            engine.draw();
        });
    }

};




} // namespace josh
