#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "Resource.hpp"
#include <cstdint>


namespace josh {


struct MaterialDiffuse {
    SharedConstTexture2D texture;
    ResourceUsage        usage;
    uintptr_t            aba_tag{};
};

struct MaterialSpecular {
    SharedConstTexture2D texture;
    ResourceUsage        usage;
    GLfloat              shininess{ 128.f };
    uintptr_t            aba_tag{};
};

struct MaterialNormal {
    SharedConstTexture2D texture;
    ResourceUsage        usage;
    uintptr_t            aba_tag{};
};


} // namespace josh
