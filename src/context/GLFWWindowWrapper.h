#pragma once
#include <stdexcept>
#include <utility>
#include <memory>
#include <glfw3_noinclude.h>
#include "GLFWInitTerminateWrapper.h"
#include "IContextWrapper.h"

enum class GLFWOpenGLProfile {
	any = GLFW_OPENGL_ANY_PROFILE,
	core = GLFW_OPENGL_CORE_PROFILE,
	compatibility = GLFW_OPENGL_COMPAT_PROFILE
};

struct WindowSize { int width; int height; };

class GLFWWindowWrapper : public IContextWrapper {
private:
	GLFWwindow* window_{};
	std::shared_ptr<GLFWInitTerminateWrapper> raiiWrapper_;

public:
	GLFWWindowWrapper(std::shared_ptr<GLFWInitTerminateWrapper> raiiWrapper,
					  int initWidth, int initHeight, const char* title,
					  int contextVerMajor, int contextVerMinor,
					  GLFWOpenGLProfile profile,
					  GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr)
					  : raiiWrapper_{ std::move(raiiWrapper) } {

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, contextVerMajor);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, contextVerMinor);
		glfwWindowHint(GLFW_OPENGL_PROFILE, static_cast<int>(profile));

		window_ = glfwCreateWindow(initWidth, initHeight, title, monitor, share);
		if ( !window_ ) {
			throw std::runtime_error("runtime_error: failed to create GLFW window");
		}

		makeContextCurrent();
	}

	void makeContextCurrent() const { glfwMakeContextCurrent(window_); }

	void setFramebufferSizeCallback(GLFWframebuffersizefun callback) {
		glfwSetFramebufferSizeCallback(window_, callback);
	}

	WindowSize getWindowSize() {
		int w, h;
		glfwGetWindowSize(window_, &w, &h);
		return { w, h };
	}

	void swapBuffers() { glfwSwapBuffers(window_); }

	// implicit cast to window*
	operator GLFWwindow*() { return window_; }


public:
	virtual ~GLFWWindowWrapper() override = default;
};
