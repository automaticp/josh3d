#include "All.hpp"
#include "FrameTimer.hpp"
#include "Game.hpp"
#include "ShaderSource.hpp"
#include "Sprite.hpp"
#include "glfwpp/event.h"

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

    learn::FrameTimer timer;
    SpriteRenderer renderer{
        learn::ShaderSource{learn::FileReader{}, "src/breakout/shaders/sprite.vert"},
        learn::ShaderSource{learn::FileReader{}, "src/breakout/shaders/sprite.frag"}
    };
    Game game{ timer, renderer };

    game.init();

    while (!window.shouldClose()) {
        timer.update();

        glfw::pollEvents();
        // Process input?

        glClearColor(0.4, 0.2, 0.15, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        game.render();

        window.swapBuffers();
    }

}
