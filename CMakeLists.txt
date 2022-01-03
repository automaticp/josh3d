cmake_minimum_required(VERSION 3.20)
project(ogl-learning)

#file(GLOB current_sources CONFIGURE_DEPENDS *.h *.cpp)

add_executable(ogllearn main.cpp)

add_subdirectory(external/stb)
add_subdirectory(external/glm)
add_subdirectory(external/glad)
add_subdirectory(external/glfw)

target_compile_features(ogllearn PRIVATE cxx_std_20)

target_link_libraries(ogllearn 
	PRIVATE 
		stb::stb 
		glm::glm 
		glad::glad 
		glfw::glfw
)