
set(
    EXTERNAL_DIRS
        glbinding
        glfwpp
        glm
        stb
        assimp
        range-v3
        imgui
)

foreach(EXTERNAL_DIR ${EXTERNAL_DIRS})
    add_subdirectory(${EXTERNAL_DIR})
endforeach()

