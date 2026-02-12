#pragma once
#include "StageContext.hpp"


namespace josh {


struct FrustumCulling
{
    void operator()(PrecomputeContext context);
};


} // namespace josh
