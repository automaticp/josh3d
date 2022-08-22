add_library(learnlib STATIC)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS *.h *.hpp *.cpp)
target_sources(learnlib PRIVATE "${LOCAL_SOURCES}")
target_include_directories(learnlib PUBLIC .)
target_compile_features(learnlib PUBLIC cxx_std_20)


# external dependencies
target_link_libraries(learnlib
    PUBLIC
        glm::glm
        glfwpp::glfwpp
        glbinding::glbinding
        glbinding::glbinding-aux
        stb::stb
        assimp::assimp
)
