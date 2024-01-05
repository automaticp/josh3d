#pragma once
#include "stages/OverlaySelectedStage.hpp"


namespace josh::imguihooks {


class OverlaySelectedStageHook {
private:
    OverlaySelectedStage& stage_;

public:
    OverlaySelectedStageHook(OverlaySelectedStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
