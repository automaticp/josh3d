# ogl-learning
The project I am using to learn the basics of OpenGL. Follows https://learnopengl.com/ tutorial.

# requirements

[`vcpkg`](https://github.com/microsoft/vcpkg)

# build

Configure cmake from the project root:

    mkdir build
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake

And build:

    cmake --build build

# run

Run executable from the project root directory.
