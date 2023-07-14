cmake_minimum_required(VERSION 3.20)
project(josh3d)

# add external packages as IMPORTED GLOBAL targets
add_subdirectory(external)

add_subdirectory(src)

add_subdirectory(tests)
