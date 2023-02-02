#include "Game.hpp"
#include "All.hpp"

#include <glfwpp/glfwpp.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>


// Things I want in breakout 2:
//
// ECS
// Fixed tick based timing and physics
// Better collision resolution
// Event-based input handling



int main() {
    using namespace gl;
    using namespace learn;

    auto glfw_instance{ glfw::init() };

	glfw::WindowHints{
		.contextVersionMajor=4, .contextVersionMinor=3, .openglProfile=glfw::OpenGlProfile::Core
	}.apply();
	glfw::Window window{ 800, 600, "Breakout2" };
	glfw::makeContextCurrent(window);
	// glfw::swapInterval(0);
	// window.setInputModeCursor(glfw::CursorMode::Disabled);

	glbinding::initialize(glfwGetProcAddress);

    globals::RAIIContext global_context;
    globals::window_size.track(window);

#ifndef NDEBUG
	learn::enable_glbinding_logger();
#endif

    window.framebufferSizeEvent.setCallback(
        [](glfw::Window&, int w, int h) {
            globals::window_size.set_to(w, h);
            glViewport(0, 0, w, h);
        }
    );

    auto [width, height] = globals::window_size.size();

    glViewport(0, 0, width, height);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Game game{ window };

    while (!window.shouldClose()) {
        globals::frame_timer.update();

        glfw::pollEvents();
        game.process_input();

        game.update();

        glClearColor(0.5, 0.0, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        game.render();

        window.swapBuffers();
    }

}
