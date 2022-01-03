
find_package(glfw3 CONFIG REQUIRED)

set_target_properties(glfw PROPERTIES IMPORTED_GLOBAL TRUE)

add_library(glfw::glfw ALIAS glfw)
