#pragma once
#include "stages/PostprocessHDREyeAdaptationStage.hpp"



namespace learn::imguihooks {


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


} // namespace learn::imguihooks
