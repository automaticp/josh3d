#pragma once
#include "RenderEngine.hpp"
#include "LightCasters.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>







namespace learn {

// Clears the color buffer with the color of the ambient light.
// Peak rendering.
class AmbientBackgroundStage {
public:
    void operator()(const RenderEngine& engine, entt::registry& registry) {
        using namespace gl;

        for (auto [_, ambi] : registry.view<const light::Ambient>().each()) {
            glClearColor(ambi.color.r, ambi.color.g, ambi.color.b, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }



};


} // namespace learn
