#pragma once
#include "stages/PostprocessHDRStage.hpp"




namespace josh::imguihooks {


class PostprocessHDRStageHook {
private:
    PostprocessHDRStage& stage_;

public:
    PostprocessHDRStageHook(PostprocessHDRStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};



} // namespace josh::imguihooks
