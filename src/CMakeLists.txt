add_executable(josh3d-demo main.cpp)
target_include_directories(josh3d-demo PRIVATE .)
target_compile_features(josh3d-demo PRIVATE cxx_std_20)

add_subdirectory(josh3d)

# internal dependencies
target_link_libraries(josh3d-demo
    PRIVATE
        josh3d
)

# external depenencies
target_link_libraries(josh3d-demo
    PRIVATE
        stb::stb
        glm::glm
        glbinding::glbinding
        glbinding::glbinding-aux
        glfwpp::glfwpp
        assimp::assimp
        imgui::imgui
)
