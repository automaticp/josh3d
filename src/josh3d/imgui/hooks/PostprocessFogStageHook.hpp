#pragma once
#include "stages/PostprocessFogStage.hpp"



namespace josh::imguihooks {


class PostprocessFogStageHook {
private:
    PostprocessFogStage& stage_;

public:
    PostprocessFogStageHook(PostprocessFogStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks


