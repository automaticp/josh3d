add_library(world INTERFACE)
target_sources(world INTERFACE Basis.h)
target_include_directories(world INTERFACE .)

target_link_libraries(world
    INTERFACE
        glm::glm
)
