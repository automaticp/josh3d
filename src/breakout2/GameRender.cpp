#include "Game.hpp"
#include <glbinding/gl/gl.h>

void Game::render() {
    imgui_.new_frame();

    renderer_.draw(registry_, fx_manager_);

    update_gui();

    imgui_.render();
}
