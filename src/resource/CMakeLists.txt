add_library(resource INTERFACE)
target_sources(resource INTERFACE IResource.h Shader.h ShaderProgram.h Texture.h VAO.h VBO.h)
target_include_directories(resource INTERFACE .)

# internal dependencies
target_link_libraries(resource
    INTERFACE
        types
)

# external dependencies
target_link_libraries(resource
    INTERFACE
        glm::glm
        glad::glad
        stb::stb
)
