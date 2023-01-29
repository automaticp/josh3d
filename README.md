# ogl-learning
The project I am using to learn OpenGL and 3D rendering. Follows https://learnopengl.com/ tutorial for GL things, while I build a small rendering library around that.

# Requirements

C++20, [CMake](https://cmake.org), [vcpkg](https://github.com/microsoft/vcpkg)

# Dependencies

[glbinding](https://github.com/cginternals/glbinding),
[glfw](https://github.com/glfw/glfw),
[glfwpp](https://github.com/janekb04/glfwpp),
[glm](https://github.com/g-truc/glm),
[assimp](https://github.com/assimp/assimp),
[stb](https://github.com/nothings/stb),
[range-v3](https://github.com/ericniebler/range-v3),
[imgui](https://github.com/ocornut/imgui)

All are satisfied automatically with vcpkg or FetchContent.

# Build

Configure and build from the project root:

    mkdir build

    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake

    cmake --build build


# Run

Run executable from the project root.
