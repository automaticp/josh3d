#pragma once
#include "StageContext.hpp"


namespace josh {


struct BoundingVolumeResolution
{
    void operator()(PrecomputeContext context);
};


} // namespace josh
