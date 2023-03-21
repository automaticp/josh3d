#include "Game.hpp"
#include <glbinding/gl/gl.h>

void Game::render() {
    imgui_.new_frame();

    vfx_renderer_.draw(
        [this] {
            using namespace gl;
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            renderer_.draw_sprites(registry_);
        },
        fx_manager_
    );

    update_gui();

    imgui_.render();
}
