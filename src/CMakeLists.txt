add_executable(ogllearn main.cpp)
target_compile_features(ogllearn PRIVATE cxx_std_20)

add_subdirectory(learnlib)

# internal dependencies
target_link_libraries(ogllearn
    PRIVATE
        learnlib
)

# external depenencies
target_link_libraries(ogllearn
    PRIVATE
        stb::stb
        glm::glm
        glbinding::glbinding
        glbinding::glbinding-aux
        glfwpp::glfwpp
        assimp::assimp
)
