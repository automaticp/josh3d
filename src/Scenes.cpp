#include <assimp/Importer.hpp>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/functions.h>
#include <glm/geometric.hpp>
#include <numeric>
#include <glfwpp/glfwpp.h>
#include <glbinding/gl/gl.h>
#include "AssimpModelLoader.hpp"
#include "BoxScene.hpp"
#include "FrameTimer.hpp"
#include "GLObjects.hpp"
#include "Globals.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "Scenes.hpp"
#include "All.hpp"
#include "ShaderBuilder.hpp"
#include "ShaderSource.hpp"
#include "Transform.hpp"
#include "ModelScene.hpp"
#include "PostprocessingScene.hpp"
#include <glfwpp/window.h>


void render_model_scene(glfw::Window& window) {

    ModelScene scene{ window };

    while (!window.shouldClose()) {
        learn::globals::frame_timer.update();

        glfw::pollEvents();
        scene.process_input();

        scene.update();

        scene.render();

        window.swapBuffers();
    }

}

void render_postprocessing_scene(glfw::Window& window) {

    PostprocessingScene scene{ window };

    while (!window.shouldClose()) {
        learn::globals::frame_timer.update();

        glfw::pollEvents();
        scene.process_input();

        scene.update();

        scene.render();

        window.swapBuffers();
    }

}


void render_box_scene(glfw::Window& window) {

    BoxScene scene{ window };

    while (!window.shouldClose()) {
        learn::globals::frame_timer.update();

        glfw::pollEvents();
        scene.process_input();

        scene.update();

        scene.render();

        window.swapBuffers();
    }

}


