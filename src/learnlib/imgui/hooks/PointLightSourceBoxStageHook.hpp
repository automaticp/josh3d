#pragma once
#include "PointLightSourceBoxStage.hpp"



namespace learn::imguihooks {


class PointLightSourceBoxStageHook {
private:
    PointLightSourceBoxStage& stage_;

public:
    PointLightSourceBoxStageHook(PointLightSourceBoxStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace learn::imguihooks
