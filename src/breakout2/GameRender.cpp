#include "Game.hpp"


void Game::render() {
    imgui_.new_frame();

    renderer_.draw_sprites(registry_);

    update_gui();

    imgui_.render();
}
