#pragma once
#include "stages/GBufferStage.hpp"


namespace learn::imguihooks {


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


} // namespace learn::imguihooks
