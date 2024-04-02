#pragma once
#include "Shared.hpp"
#include "GLObjects.hpp"


namespace josh::components {


struct Skybox {
    Shared<dsa::UniqueCubemap> cubemap;
};


} // namespace josh::components
