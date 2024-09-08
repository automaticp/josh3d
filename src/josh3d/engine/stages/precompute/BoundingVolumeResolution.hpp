#pragma once
#include "RenderEngine.hpp"


namespace josh::stages::precompute {


class BoundingVolumeResolution {
public:
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh::stages::precompute
