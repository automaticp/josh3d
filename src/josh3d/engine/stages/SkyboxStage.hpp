#pragma once
#include "RenderEngine.hpp"
#include "SkyboxRenderer.hpp"
#include "RenderComponents.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>




namespace josh {



class SkyboxStage {
private:
    SkyboxRenderer renderer_;

public:

    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry)
    {
        using namespace gl;

        glm::mat4 projection = engine.camera().projection_mat();
        glm::mat4 view       = engine.camera().view_mat();

        engine.draw([&, this] {

            for (auto [e, skybox] : registry.view<const components::Skybox>().each()) {
                renderer_.draw(*skybox.cubemap, projection, view);
            }

        });


    }


};


} // namspace josh
