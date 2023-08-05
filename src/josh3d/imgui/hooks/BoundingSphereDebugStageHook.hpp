#pragma once
#include "stages/BoundingSphereDebugStage.hpp"


namespace josh::imguihooks {


class BoundingSphereDebugStageHook {
private:
    BoundingSphereDebugStage& stage_;

public:
    BoundingSphereDebugStageHook(BoundingSphereDebugStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
