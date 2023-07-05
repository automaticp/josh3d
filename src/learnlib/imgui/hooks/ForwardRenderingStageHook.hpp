#pragma once
#include "ForwardRenderingStage.hpp"



namespace learn::imguihooks {


class ForwardRenderingStageHook {
private:
    ForwardRenderingStage& stage_;

public:
    ForwardRenderingStageHook(ForwardRenderingStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace learn::imguihooks
