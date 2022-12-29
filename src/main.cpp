#include <numbers>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glfwpp/glfwpp.h>

#include "Scenes.hpp"
#include "All.hpp"


int main() {
	using namespace gl;
	using namespace learn;

	// Init GLFW and create a window
	auto glfw_instance{ glfw::init() };

	glfw::WindowHints{
		.contextVersionMajor=3, .contextVersionMinor=3, .openglProfile=glfw::OpenGlProfile::Core
	}.apply();
	glfw::Window window{ 800, 600, "WindowName" };
	window.framebufferSizeEvent.setCallback([](glfw::Window& window, int w, int h) { glViewport(0, 0, w, h); });
	glfw::makeContextCurrent(window);
	glfw::swapInterval(0);
	window.setInputModeCursor(glfw::CursorMode::Disabled);

	// Init glbindings
	glbinding::initialize(glfwGetProcAddress);
#ifndef NDEBUG
	enable_glbinding_logger();
#endif
	auto [width, height] { window.getSize() };
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);


	render_cube_scene(window);
	// render_model_scene(window);

	global_texture_handle_pool.clear_unused();
	return 0;
}


