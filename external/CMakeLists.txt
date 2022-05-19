
set(
    EXTERNAL_DIRS
        glbinding
        glfwpp
        glm
        stb
)

foreach(EXTERNAL_DIR ${EXTERNAL_DIRS})
    add_subdirectory(${EXTERNAL_DIR})
endforeach()

