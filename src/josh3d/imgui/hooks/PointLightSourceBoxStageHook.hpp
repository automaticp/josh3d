#pragma once
#include "stages/PointLightSourceBoxStage.hpp"



namespace josh::imguihooks {


class PointLightSourceBoxStageHook {
private:
    PointLightSourceBoxStage& stage_;

public:
    PointLightSourceBoxStageHook(PointLightSourceBoxStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
