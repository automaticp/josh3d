#include <numbers>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glfwpp/glfwpp.h>

#include "Globals.hpp"
#include "Scenes.hpp"
#include "All.hpp"


int main() {
	using namespace gl;
	using namespace learn;

	// Init GLFW and create a window
	auto glfw_instance{ glfw::init() };

	glfw::WindowHints{
		.scaleToMonitor=true,
        .contextVersionMajor=3, .contextVersionMinor=3,
        .openglProfile=glfw::OpenGlProfile::Core
	}.apply();
	glfw::Window window{ 800, 600, "WindowName" };
	glfw::makeContextCurrent(window);
	glfw::swapInterval(0);
	window.setInputModeCursor(glfw::CursorMode::Disabled);

	// Init glbindings
	glbinding::initialize(glfwGetProcAddress);
#ifndef NDEBUG
	enable_glbinding_logger();
#endif

    globals::window_size.track(window);

    window.framebufferSizeEvent.setCallback(
        [](glfw::Window& window, int w, int h) {
            globals::window_size.set_to(w, h);
            glViewport(0, 0, w, h);
        }
    );

    auto [width, height] { globals::window_size.size() };
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);

    render_postprocessing_scene(window);
	// render_cube_scene(window);
	// render_model_scene(window);

	learn::globals::clear_all();
	return 0;
}


