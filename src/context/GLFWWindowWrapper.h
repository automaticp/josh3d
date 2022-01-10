#pragma once
#include <GLFW/glfw3.h>
#include "IContextWrapper.h"

class GLFWWindowWrapper : public IContextWrapper {


public:
	virtual ~GLFWWindowWrapper() override {
		glfwTerminate();
	}
};
