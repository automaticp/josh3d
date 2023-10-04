#pragma once
#include "stages/SkyboxStage.hpp"



namespace josh::imguihooks {


class SkyboxStageHook {
private:
    SkyboxStage& stage_;

public:
    SkyboxStageHook(SkyboxStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks

