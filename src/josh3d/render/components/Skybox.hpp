#pragma once
#include "Shared.hpp"
#include "GLObjects.hpp"


namespace josh::components {


struct Skybox {
    Shared<UniqueCubemap> cubemap;
};


} // namespace josh::components
