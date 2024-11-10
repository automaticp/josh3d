add_executable            (josh3d-demo DemoScene.cpp main.cpp)
target_include_directories(josh3d-demo PRIVATE ./)
target_compile_features   (josh3d-demo PRIVATE cxx_std_20)

add_subdirectory(josh3d)

target_link_libraries(josh3d-demo PRIVATE josh3d::josh3d)
target_link_libraries(josh3d-demo PRIVATE glfwpp::glfwpp cxxopts::cxxopts)
