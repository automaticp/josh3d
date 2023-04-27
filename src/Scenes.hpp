#pragma once
#include "GlobalsUtil.hpp"
#include "BoxScene.hpp"
#include "DirPointShadowScene.hpp"
#include <glfwpp/window.h>


template<typename SceneT>
void render_generic_scene(glfw::Window& window) {

    SceneT scene{ window };

    while (!window.shouldClose()) {
        learn::globals::frame_timer.update();

        glfw::pollEvents();
        scene.process_input();

        scene.update();

        scene.render();

        window.swapBuffers();
    }

}
