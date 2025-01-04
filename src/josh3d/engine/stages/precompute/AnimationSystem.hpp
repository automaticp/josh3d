#pragma once
#include "RenderEngine.hpp"


namespace josh::stages::precompute {


/*
Temporary system to advance animations and compute sample poses.

This must be reworked later.
*/
class AnimationSystem {
public:
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh::stages::precompute
