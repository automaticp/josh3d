cmake_minimum_required(VERSION 3.20)
project(ogl-learning)

# add external packages as IMPORTED GLOBAL targets
add_subdirectory(external/stb)
add_subdirectory(external/glm)
add_subdirectory(external/glad)
add_subdirectory(external/glfw)

# add main executable
add_subdirectory(src)
