include(FetchContent)

FetchContent_Declare(
    GLFWPP
    GIT_REPOSITORY https://github.com/janekb04/glfwpp.git
    GIT_TAG        207096552976035ae466382df6b5fc6905d06fbd
)

set(GLFWPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(GLFWPP)

# Removes the system header #include <GL/gl.h> from GLFW,
# as otherwise it causes linker errors alongside glbindings.
target_compile_definitions(GLFWPP INTERFACE GLFW_INCLUDE_NONE)

add_library(glfwpp::glfwpp ALIAS GLFWPP)
