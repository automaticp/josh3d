# ogl-learning
The project I am using to learn the basics of OpenGL. Follows https://learnopengl.com/ tutorial.

# dependencies
`glad` `glfw` `glm` `stb`

Make sure that the dependencies can be found with `find_package()`

For example, you can install these packages with `vcpkg`
and provide the toolchain file to cmake with configuration option:

    -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/sctipts/buildsystems/vcpkg.cmake

# build

Configure cmake from the project root: 
    
    mkdir build
    cmake -S . -B build

And build:

    cmake --build build

# run

Run executable from the project root directory. 
