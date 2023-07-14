#pragma once
#include "stages/DeferredShadingStage.hpp"


namespace josh::imguihooks {


class DeferredShadingStageHook {
private:
    DeferredShadingStage& stage_;

public:
    DeferredShadingStageHook(DeferredShadingStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
