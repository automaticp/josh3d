#pragma once
#include "RenderEngine.hpp"


namespace josh {


/*
Temporary system to advance animations and compute sample poses.

This must be reworked later.
*/
struct AnimationSystem
{
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh
