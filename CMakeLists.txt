cmake_minimum_required(VERSION 3.20)
project(ogl-learning)

# add external packages as IMPORTED GLOBAL targets
add_subdirectory(external)

# add main executable
add_subdirectory(src)

# add tests
add_subdirectory(tests)
