# === doctest ===

find_package(doctest CONFIG REQUIRED)
set_target_properties(doctest::doctest PROPERTIES IMPORTED_GLOBAL TRUE)


# === Assimp ===

find_package(assimp CONFIG REQUIRED)
set_target_properties(assimp::assimp PROPERTIES IMPORTED_GLOBAL TRUE)



# === entt ===

find_package(EnTT CONFIG REQUIRED)
set_target_properties(EnTT::EnTT PROPERTIES IMPORTED_GLOBAL TRUE)



# === glbinding ===

find_package(glbinding CONFIG REQUIRED)
set_target_properties(glbinding::glbinding PROPERTIES IMPORTED_GLOBAL TRUE)
set_target_properties(glbinding::glbinding-aux PROPERTIES IMPORTED_GLOBAL TRUE)



# === glfwpp ===

include(FetchContent)

FetchContent_Declare(
    GLFWPP
    GIT_REPOSITORY https://github.com/automaticp/glfwpp.git
    GIT_TAG        8bd4c23d367cf74eb1770e473f8bcb7432a2bd4d
)

set(GLFWPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(GLFWPP)

# Removes the system header #include <GL/gl.h> from GLFW,
# as otherwise it causes linker errors alongside glbindings.
target_compile_definitions(GLFWPP INTERFACE GLFW_INCLUDE_NONE)

add_library(glfwpp::glfwpp ALIAS GLFWPP)



# === glm ===

find_package(glm CONFIG REQUIRED)
set_target_properties(glm::glm PROPERTIES IMPORTED_GLOBAL TRUE)



# === dear-imgui ===

find_package(imgui CONFIG REQUIRED)
set_target_properties(imgui::imgui PROPERTIES IMPORTED_GLOBAL TRUE)



# === range-v3 ===

find_package(range-v3 CONFIG REQUIRED)
set_target_properties(range-v3::range-v3 PROPERTIES IMPORTED_GLOBAL TRUE)
set_target_properties(range-v3::concepts PROPERTIES IMPORTED_GLOBAL TRUE)



# === stb ===

# only provides Stb_INCLUDE_DIR, no target
find_package(Stb REQUIRED)

# so we make our own
add_library(stb::stb IMPORTED INTERFACE GLOBAL)
target_include_directories(stb::stb INTERFACE ${Stb_INCLUDE_DIR})

# this creates a single translation unit with all the relevant stb implementations;
# no longer have to #define STB_..._IMPLEMENTATION in the project's .cpp files
add_library(stb_implementation STATIC)
target_sources(stb_implementation PRIVATE stb_implementation.c)

# get INCLUDE_DIR from stb to #include <stb_image.h>
target_link_libraries(stb_implementation PRIVATE stb::stb)

# require anyone that depends on stb to link against stb_implementation
target_link_libraries(stb::stb INTERFACE stb_implementation)
