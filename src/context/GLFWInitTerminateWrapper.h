#pragma once
#include <glfw3_noinclude.h>

class GLFWInitTerminateWrapper {
// Simple RAII wrapper for glfw runtime.
// std::make_shared<GLFWInitTerminateWrapper> in main
// and share ownership with each glfw window.
public:
    GLFWInitTerminateWrapper() {
        glfwInit();
    }

    ~GLFWInitTerminateWrapper() {
        glfwTerminate();
    }
};
