#pragma once
#include "PostprocessHDRStage.hpp"




namespace learn::imguihooks {


class PostprocessHDRStageHook {
private:
    PostprocessHDRStage& stage_;

public:
    PostprocessHDRStageHook(PostprocessHDRStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};



} // namespace learn::imguihooks
