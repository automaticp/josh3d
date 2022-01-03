# only provides Stb_INCLUDE_DIR, no target
find_package(Stb REQUIRED)

add_library(stb::stb IMPORTED INTERFACE GLOBAL)
target_include_directories(stb::stb INTERFACE ${Stb_INCLUDE_DIR})

# this creates a single translation unit with all the relevant stb implementations;
# no longer have to #define STB_..._IMPLEMENTATION in the project's .cpp files
add_library(stb_implementation STATIC)
target_sources(stb_implementation PRIVATE stb_implementation.cpp)

# get INCLUDE_DIR from stb to #include <stb_image.h>
target_link_libraries(stb_implementation PRIVATE stb::stb)

# require anyone that depends on stb to link against stb_implementation
target_link_libraries(stb::stb INTERFACE stb_implementation)
