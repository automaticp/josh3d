#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glfwpp/glfwpp.h>

#include "GlobalsUtil.hpp"
#include "DemoScene.hpp"



template<typename SceneT>
void render_generic_scene(glfw::Window& window) {

    SceneT scene{ window };

    while (!window.shouldClose()) {
        josh::globals::frame_timer.update();

        glfw::pollEvents();
        scene.process_input();

        scene.update();

        scene.render();

        window.swapBuffers();
    }

}




int main() {
    using namespace gl;
    using namespace josh;


    auto glfw_instance{ glfw::init() };

    glfw::WindowHints{
        .scaleToMonitor=true,
        .contextVersionMajor=4, .contextVersionMinor=3,
        .openglProfile=glfw::OpenGlProfile::Core
    }.apply();

    glfw::Window window{ 800, 600, "WindowName" };
    glfw::makeContextCurrent(window);
    glfw::swapInterval(0);
    window.setInputModeCursor(glfw::CursorMode::Normal);


    glbinding::initialize(glfwGetProcAddress);
#ifndef NDEBUG
    enable_glbinding_logger();
#endif


    globals::RAIIContext globals_context;
    globals::window_size.track(window);


    window.framebufferSizeEvent.setCallback(
        [](glfw::Window& /* window */, int w, int h) {
            globals::window_size.set_to(w, h);
            glViewport(0, 0, w, h);
        }
    );

    auto [width, height] = globals::window_size.size();
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);


    // render_generic_scene<BoxScene3>(window);
    render_generic_scene<DemoScene>(window);


    return 0;
}


