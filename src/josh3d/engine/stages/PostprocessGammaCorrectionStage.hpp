#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "VirtualFilesystem.hpp"
#include <entt/fwd.hpp>
#include <glbinding/gl/gl.h>




namespace josh {

class PostprocessGammaCorrectionStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_gamma.frag"))
            .get()
    };

public:
    float gamma{ 2.2f };
    bool use_srgb{ true };

    void operator()(const RenderEnginePostprocessInterface& engine, const entt::registry&) {

        using namespace gl;

        sp_.use().and_then([&, this](ActiveShaderProgram& ashp) {
            engine.screen_color().bind_to_unit(GL_TEXTURE0);
            ashp.uniform("color", 0);

            if (use_srgb) {
                ashp.uniform("gamma", 1.0f);
                glEnable(GL_FRAMEBUFFER_SRGB);
                engine.draw();
                glDisable(GL_FRAMEBUFFER_SRGB);
            } else /* custom gamma */ {
                ashp.uniform("gamma", gamma);
                engine.draw();
            }

        });


    }

};




} // namespace josh
