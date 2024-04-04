cmake_minimum_required(VERSION 3.21)
project(josh3d)

option(JOSH3D_BUILD_TESTS     "Build tests"                                       OFF)
option(JOSH3D_USE_PCH         "Use precompiled headers when building the library" ON )
option(JOSH3D_BUILD_WITH_ASAN "Enable AddressSanitizer when building the library" OFF)

# add external packages as IMPORTED GLOBAL targets
add_subdirectory(external)

add_subdirectory(src)


if (JOSH3D_BUILD_TESTS)
    add_subdirectory(tests)
endif()
