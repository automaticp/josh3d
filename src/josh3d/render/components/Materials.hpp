#pragma once
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"


namespace josh::components {


struct MaterialDiffuse {
    SharedConstTexture2D texture;
};

struct MaterialSpecular {
    SharedConstTexture2D texture;
    GLfloat              shininess{ 128.f };
};

struct MaterialNormal {
    SharedConstTexture2D texture;
};


} // namespace josh::components
