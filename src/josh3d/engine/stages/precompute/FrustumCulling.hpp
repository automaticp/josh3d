#pragma once
#include "RenderEngine.hpp"


namespace josh {


struct FrustumCulling
{
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh
