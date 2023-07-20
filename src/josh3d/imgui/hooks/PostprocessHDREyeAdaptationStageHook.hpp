#pragma once
#include "stages/PostprocessHDREyeAdaptationStage.hpp"



namespace josh::imguihooks {


class PostprocessHDREyeAdaptationStageHook {
private:
    PostprocessHDREyeAdaptationStage& stage_;

public:
    PostprocessHDREyeAdaptationStageHook(
        PostprocessHDREyeAdaptationStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
