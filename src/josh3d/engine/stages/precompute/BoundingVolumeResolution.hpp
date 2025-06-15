#pragma once
#include "RenderEngine.hpp"


namespace josh {


struct BoundingVolumeResolution
{
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh
