#pragma once
#include "stages/OverlayGBufferDebugStage.hpp"


namespace josh::imguihooks {


class OverlayGBufferDebugStageHook {
private:
    OverlayGBufferDebugStage& stage_;

public:
    OverlayGBufferDebugStageHook(OverlayGBufferDebugStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
