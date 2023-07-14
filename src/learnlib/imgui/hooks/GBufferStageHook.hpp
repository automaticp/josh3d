#pragma once
#include "stages/GBufferStage.hpp"


namespace josh::imguihooks {


class GBufferStageHook {
private:
    GBufferStage& stage_;
    SharedStorageView<GBuffer> gbuffer_;

public:
    GBufferStageHook(GBufferStage& stage)
        : stage_{ stage }
        , gbuffer_{ stage_.get_read_view() }
    {}

    void operator()();
};


} // namespace josh::imguihooks
