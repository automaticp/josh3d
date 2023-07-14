#pragma once
#include "stages/PostprocessGammaCorrectionStage.hpp"


namespace josh::imguihooks {


class PostprocessGammaCorrectionStageHook {
private:
    PostprocessGammaCorrectionStage& stage_;

public:
    PostprocessGammaCorrectionStageHook(PostprocessGammaCorrectionStage& stage)
        : stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
