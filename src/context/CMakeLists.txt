add_library(context INTERFACE)
target_sources(context INTERFACE IContextWrapper.h)
target_include_directories(context INTERFACE .)

# external dependencies
target_link_libraries(context
    INTERFACE
        glfw::glfw
)
