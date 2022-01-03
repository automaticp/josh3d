
find_package(Stb REQUIRED)

add_library(stb::stb IMPORTED INTERFACE GLOBAL)

target_include_directories(stb::stb INTERFACE ${Stb_INCLUDE_DIR})