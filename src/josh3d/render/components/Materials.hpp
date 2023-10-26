#pragma once
#include "Shared.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"


namespace josh::components {


struct MaterialDiffuse {
    Shared<UniqueTexture2D> diffuse;
};

struct MaterialSpecular {
    Shared<UniqueTexture2D> specular;
    GLfloat shininess{ 128.f };
};

struct MaterialNormal {
    Shared<UniqueTexture2D> normal;
};


} // namespace josh::components
