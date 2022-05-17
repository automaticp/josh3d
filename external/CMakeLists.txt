
set(
    EXTERNAL_DIRS
        glbinding
        glfw
        glm
        stb
)

foreach(EXTERNAL_DIR ${EXTERNAL_DIRS})
    add_subdirectory(${EXTERNAL_DIR})
endforeach()

