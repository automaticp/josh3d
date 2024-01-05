#pragma once
#include "stages/primary/GBufferStorage.hpp"


namespace josh::imguihooks::primary {


class GBufferStorage {
private:
    stages::primary::GBufferStorage& stage_;
    SharedStorageView<GBuffer> gbuffer_;

public:
    GBufferStorage(stages::primary::GBufferStorage& stage)
        : stage_{ stage }
        , gbuffer_{ stage_.get_read_view() }
    {}

    void operator()();
};


} // namespace josh::imguihooks::primary
