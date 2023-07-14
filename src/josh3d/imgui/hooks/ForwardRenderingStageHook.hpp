#pragma once
#include "stages/ForwardRenderingStage.hpp"



namespace josh::imguihooks {


class ForwardRenderingStageHook {
private:
    ForwardRenderingStage& stage_;

public:
    ForwardRenderingStageHook(ForwardRenderingStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
