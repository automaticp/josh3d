include(FetchContent)

FetchContent_Declare(
    GLFWPP
    GIT_REPOSITORY https://github.com/automaticp/glfwpp.git
    GIT_TAG        8bd4c23d367cf74eb1770e473f8bcb7432a2bd4d
)

set(GLFWPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(GLFWPP)

# Removes the system header #include <GL/gl.h> from GLFW,
# as otherwise it causes linker errors alongside glbindings.
target_compile_definitions(GLFWPP INTERFACE GLFW_INCLUDE_NONE)

add_library(glfwpp::glfwpp ALIAS GLFWPP)
