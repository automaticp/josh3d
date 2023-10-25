add_library(josh3d STATIC)
file(GLOB_RECURSE LOCAL_SOURCES CONFIGURE_DEPENDS *.cpp)
target_sources(josh3d PRIVATE "${LOCAL_SOURCES}")
target_include_directories(josh3d
    PUBLIC
        ./
        ./data/
        ./filesystem/
        ./gl/
        ./imgui/
        ./render/
        ./resource/
        ./scene/
        ./util/
        ./engine/
)
target_compile_features(josh3d PUBLIC cxx_std_20)


# external dependencies
target_link_libraries(josh3d
    PUBLIC
        glm::glm
        glfwpp::glfwpp
        glbinding::glbinding
        glbinding::glbinding-aux
        stb::stb
        assimp::assimp
        EnTT::EnTT
        range-v3::range-v3
        range-v3::concepts
)

if (JOSH3D_USE_PCH)
    target_precompile_headers(josh3d
        PRIVATE
            <glbinding/gl/gl.h>
            <imgui.h>
            <imgui_stdlib.h>
            <glm/glm.hpp>
            <glm/ext.hpp>
            <entt/entt.hpp>
    )
endif()
