#pragma once
#include "stages/PostprocessBloomStage.hpp"



namespace learn::imguihooks {


class PostprocessBloomStageHook {
private:
    PostprocessBloomStage& stage_;

public:
    PostprocessBloomStageHook(PostprocessBloomStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace learn::imguihooks
