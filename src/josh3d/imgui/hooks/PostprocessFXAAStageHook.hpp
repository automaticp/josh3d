#pragma once
#include "stages/PostprocessFXAAStage.hpp"


namespace josh::imguihooks {


class PostprocessFXAAStageHook {
private:
    PostprocessFXAAStage& stage_;

public:
    PostprocessFXAAStageHook(PostprocessFXAAStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
