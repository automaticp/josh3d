# === boost ===
# Boost::iostreams
find_package(Boost REQUIRED COMPONENTS iostreams)


# === fmt ===
# fmt::fmt
# fmt::fmt-header-only
find_package(fmt CONFIG REQUIRED)


# === cxxopts ===
# cxxopts::cxxopts
find_package(cxxopts CONFIG REQUIRED)


# === doctest ===
# doctest::doctest
if (JOSH3D_BUILD_TESTS)
    find_package(doctest CONFIG REQUIRED)
endif()

# === Assimp ===
# assimp::assimp
find_package(assimp CONFIG REQUIRED)


# === entt ===
# EnTT::EnTT
find_package(EnTT CONFIG REQUIRED)


# === glbinding ===
# glbinding::glbinding
# glbinding::glbinding-aux
find_package(glbinding CONFIG REQUIRED)


# === glfwpp ===
# glfwpp::glfwpp
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
# glm::glm
find_package(glm CONFIG REQUIRED)
target_compile_definitions(glm::glm INTERFACE GLM_ENABLE_EXPERIMENTAL)


# === dear-imgui ===
# imgui::imgui
find_package(imgui CONFIG REQUIRED)


# === Imguizmo ===
# unofficial::imguizmo::imguizmo
find_package(unofficial-imguizmo CONFIG REQUIRED)


# === range-v3 ===
# range-v3::range-v3
# range-v3::concepts
find_package(range-v3 CONFIG REQUIRED)


# === ctre ===
# ctre::ctre
find_package(ctre CONFIG REQUIRED)


# === stb ===
# stb::stb

# only provides Stb_INCLUDE_DIR, no target
find_package(Stb REQUIRED)

# so we make our own
add_library(stb::stb IMPORTED INTERFACE GLOBAL)
target_include_directories(stb::stb INTERFACE ${Stb_INCLUDE_DIR})

# this creates a single translation unit with all the relevant stb implementations;
# no longer have to #define STB_..._IMPLEMENTATION in the project's .cpp files
add_library(stb_implementation STATIC)
target_sources(stb_implementation PRIVATE ${CMAKE_SOURCE_DIR}/external/stb_implementation.c)

# get INCLUDE_DIR from stb to #include <stb_image.h>
target_link_libraries(stb_implementation PRIVATE stb::stb)

# require anyone that depends on stb to link against stb_implementation
target_link_libraries(stb::stb INTERFACE stb_implementation)


# === nlohmann-json ===
# nlohmann_json::nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)




# Precompile external headers
if (JOSH3D_USE_PCH)
    target_precompile_headers(glbinding::glbinding INTERFACE <glbinding/gl/gl.h>)
    target_precompile_headers(imgui::imgui INTERFACE <imgui.h> <imgui_stdlib.h>)
    target_precompile_headers(glm::glm INTERFACE <glm/glm.hpp> <glm/ext.hpp>)
    target_precompile_headers(EnTT::EnTT INTERFACE <entt/entt.hpp>)
endif()
