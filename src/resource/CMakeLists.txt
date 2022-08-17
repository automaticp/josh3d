add_library(resource INTERFACE)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS *.h *.hpp)
target_sources(resource INTERFACE "${LOCAL_SOURCES}")
target_include_directories(resource INTERFACE .)
target_compile_features(resource INTERFACE cxx_std_20)

# internal dependencies
target_link_libraries(resource
    INTERFACE
        types
)

# external dependencies
target_link_libraries(resource
    INTERFACE
        glm::glm
        glbinding::glbinding
        stb::stb
        assimp::assimp
)

add_library(resource_tu STATIC resource.cpp)
target_link_libraries(resource_tu PRIVATE resource)
