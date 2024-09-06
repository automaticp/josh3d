#pragma once
#include "RenderEngine.hpp"


namespace josh::stages::precompute {


class FrustumCulling {
public:
    void operator()(RenderEnginePrecomputeInterface& engine);
};


} // namespace josh::stages::precompute
