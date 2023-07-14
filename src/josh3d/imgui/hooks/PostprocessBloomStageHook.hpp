#pragma once
#include "stages/PostprocessBloomStage.hpp"



namespace josh::imguihooks {


class PostprocessBloomStageHook {
private:
    PostprocessBloomStage& stage_;

public:
    PostprocessBloomStageHook(PostprocessBloomStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
