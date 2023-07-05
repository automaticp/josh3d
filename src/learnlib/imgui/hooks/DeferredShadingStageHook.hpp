#pragma once
#include "stages/DeferredShadingStage.hpp"


namespace learn::imguihooks {


class DeferredShadingStageHook {
private:
    DeferredShadingStage& stage_;

public:
    DeferredShadingStageHook(DeferredShadingStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace learn::imguihooks
