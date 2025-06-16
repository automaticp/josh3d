# === boost ===
# Boost::iostreams
# Boost::scope
# Boost::container
# Boost::unordered
# Boost::outcome
# Boost::any
# Boost::interprocess
# Boost::uuid
find_package(Boost REQUIRED COMPONENTS iostreams container unordered scope outcome any interprocess uuid type_index)


# === fmt ===
# fmt::fmt
# fmt::fmt-header-only
find_package(fmt CONFIG REQUIRED)


# === cppcoro ===
# cppcoro::cppcoro
include(FetchContent)

FetchContent_Declare(
    cppcoro
    GIT_REPOSITORY https://github.com/andreasbuhr/cppcoro
    GIT_TAG        a4ef65281814b18fdd1ac5457d3e219347ec6cb8
)

FetchContent_MakeAvailable(cppcoro)

add_library(cppcoro::cppcoro ALIAS cppcoro)


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
target_compile_definitions(glm::glm INTERFACE GLM_FORCE_QUAT_DATA_XYZW)


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


# === libspng ===
# spng::spng
find_package(SPNG CONFIG REQUIRED)


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


# === cgltf ===
# cgltf::cgltf
find_path(CGLTF_INCLUDE_DIR "cgltf.h" REQUIRED)
add_library(cgltf::cgltf IMPORTED INTERFACE)
target_include_directories(cgltf::cgltf INTERFACE ${CGLTF_INCLUDE_DIR})

add_library(cgltf_implementation STATIC ${CMAKE_SOURCE_DIR}/external/cgltf_implementation.c)
target_link_libraries(cgltf_implementation PRIVATE cgltf::cgltf)
target_link_libraries(cgltf::cgltf INTERFACE cgltf_implementation)


# === nlohmann-json ===
# nlohmann_json::nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)


# === jsoncons ===
# jsoncons::jsoncons
find_package(jsoncons CONFIG REQUIRED)
add_library(jsoncons::jsoncons ALIAS jsoncons)


