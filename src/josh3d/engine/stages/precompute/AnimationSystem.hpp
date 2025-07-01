#pragma once
#include "StageContext.hpp"


namespace josh {


/*
Temporary system to advance animations and compute sample poses.

This must be reworked later.
*/
struct AnimationSystem
{
    void operator()(PrecomputeContext context);
};


} // namespace josh
