add_library(josh3d STATIC)
file(GLOB_RECURSE LOCAL_SOURCES CONFIGURE_DEPENDS *.cpp)
target_sources(josh3d PRIVATE "${LOCAL_SOURCES}")
target_include_directories(josh3d
    PUBLIC
        ./
        ./data/
        ./gl/
        ./imgui/
        ./render/
        ./resource/
        ./util/
        ./engine/
)
target_compile_features(josh3d PUBLIC cxx_std_20)


# external dependencies
target_link_libraries(josh3d
    PUBLIC
        glm::glm
        glfwpp::glfwpp
        glbinding::glbinding
        glbinding::glbinding-aux
        stb::stb
        assimp::assimp
        EnTT::EnTT
        range-v3::range-v3
        range-v3::concepts
)
