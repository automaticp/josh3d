add_library(input INTERFACE)
target_sources(input INTERFACE Input.h Camera.h)
target_include_directories(input INTERFACE .)

# internal dependencies
target_link_libraries(input
    INTERFACE
        world
)

# external dependencies
target_link_libraries(input
    INTERFACE
        glm::glm
        glfw::glfw
        glbinding::glbinding
)
