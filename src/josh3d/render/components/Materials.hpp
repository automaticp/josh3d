#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"


namespace josh::components {


struct MaterialDiffuse {
    SharedTexture2D texture;
};

struct MaterialSpecular {
    SharedTexture2D texture;
    GLfloat         shininess{ 128.f };
};

struct MaterialNormal {
    SharedTexture2D texture;
};


} // namespace josh::components
