#include "All.hpp"
#include "FrameTimer.hpp"
#include "Game.hpp"
#include "Globals.hpp"
#include "Input.hpp"
#include "ShaderSource.hpp"
#include "Sprite.hpp"
#include "glfwpp/event.h"
#include "glfwpp/window.h"

#include <glbinding/gl/gl.h>


int main() {
    using namespace gl;

    auto glfw_instance{ glfw::init() };

	glfw::WindowHints{
		.contextVersionMajor=3, .contextVersionMinor=3, .openglProfile=glfw::OpenGlProfile::Core
	}.apply();
	glfw::Window window{ 800, 600, "Breakout" };
	window.framebufferSizeEvent.setCallback([](glfw::Window&, int w, int h) { glViewport(0, 0, w, h); });
	glfw::makeContextCurrent(window);
	// glfw::swapInterval(0);
	// window.setInputModeCursor(glfw::CursorMode::Disabled);

	glbinding::initialize(glfwGetProcAddress);
#ifndef NDEBUG
	learn::enable_glbinding_logger();
#endif
	auto [width, height] { window.getSize() };
	glViewport(0, 0, width, height);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Game game{ window, learn::globals::frame_timer };

    game.init();

    while (!window.shouldClose()) {
        learn::globals::frame_timer.update();

        glfw::pollEvents();
        game.process_input();

        game.update();

        glClearColor(0.3, 0.35, 0.4, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        game.render();

        window.swapBuffers();
    }

    learn::globals::clear_all();

}
