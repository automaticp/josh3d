# ogl-learning
The project I am using to learn OpenGL and 3D rendering. Follows https://learnopengl.com/ tutorial for GL things, while I build a small rendering library around that.

# requirements

[CMake](https://cmake.org), [vcpkg](https://github.com/microsoft/vcpkg)

# dependencies

[glbinding](https://github.com/cginternals/glbinding),
[glfw](https://github.com/glfw/glfw),
[glfwpp](https://github.com/janekb04/glfwpp),
[glm](https://github.com/g-truc/glm),
[assimp](https://github.com/assimp/assimp),
[stb](https://github.com/nothings/stb)

# build

Configure cmake from the project root:

    mkdir build
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake

And build:

    cmake --build build

# run

Run executable from the project root.
