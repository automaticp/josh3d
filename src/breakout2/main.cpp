#include "Game.hpp"
#include "All.hpp"
#include "GlobalsUtil.hpp"

#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glfwpp/glfwpp.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>


// Things I want in breakout 2:
//
// ECS
// Fixed tick based timing and physics
// Better collision resolution
// Event-based input handling
// Event-driven game logic



int main() {
    using namespace gl;
    using namespace learn;

    auto glfw_instance{ glfw::init() };

	glfw::WindowHints{
		.contextVersionMajor=4, .contextVersionMinor=3, .openglProfile=glfw::OpenGlProfile::Core
	}.apply();
	glfw::Window window{ 1600, 900, "Breakout2" };
	glfw::makeContextCurrent(window);
	// glfw::swapInterval(1);
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
    glClearColor(0.5, 0.0, 0.5, 1.0);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    Game game{ window };

    float time_overflow{ 0.f };

    while (!window.shouldClose()) {
        globals::frame_timer.update();

        time_overflow += globals::frame_timer.delta<float>();

        while (time_overflow >= Game::update_time_step) {

            glfw::pollEvents();
            game.process_events();
            game.update();

            time_overflow -= Game::update_time_step;
        }

        game.render();

        window.swapBuffers();
    }

}
