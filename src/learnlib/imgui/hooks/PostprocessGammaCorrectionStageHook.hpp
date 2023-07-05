#pragma once
#include "stages/PostprocessGammaCorrectionStage.hpp"


namespace learn::imguihooks {


class PostprocessGammaCorrectionStageHook {
private:
    PostprocessGammaCorrectionStage& stage_;

public:
    PostprocessGammaCorrectionStageHook(PostprocessGammaCorrectionStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace learn::imguihooks
