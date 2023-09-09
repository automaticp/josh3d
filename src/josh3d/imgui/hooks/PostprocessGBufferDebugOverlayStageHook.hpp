#pragma once
#include "stages/PostprocessGBufferDebugOverlayStage.hpp"


namespace josh::imguihooks {


class PostprocessGBufferDebugOverlayStageHook {
private:
    PostprocessGBufferDebugOverlayStage& stage_;

public:
    PostprocessGBufferDebugOverlayStageHook(PostprocessGBufferDebugOverlayStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
