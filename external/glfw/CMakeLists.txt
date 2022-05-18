
find_package(glfw3 CONFIG REQUIRED)

set_target_properties(glfw PROPERTIES IMPORTED_GLOBAL TRUE)

# By default glfw includes system GL/gl.h headers,
# which define macros for certain gl types and functions.
# This conflicts with glbinding defenitions and produces linker errors.
# We need to #define GLFW_INCLUDE_NONE to prevent inclusion of system headers.
#
# Thus, glfw is wrapped in a glfw-noinclude target with a glfw3_noinclude.h header
# that adds GLFW_INCLUDE_NONE definition.
#
# Link against glfw::glfw target and #include <glfw3_noinclude.h>

add_library(glfw-noinclude INTERFACE glfw3_noinclude.h)
target_link_libraries(glfw-noinclude INTERFACE glfw)
target_include_directories(glfw-noinclude INTERFACE .)

add_library(glfw::glfw ALIAS glfw-noinclude)
