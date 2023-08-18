#pragma once
#include "SharedStorage.hpp"
#include "GLScalars.hpp"
#include "stages/PointShadowMappingStage.hpp"


namespace josh::imguihooks {


class PointShadowMappingStageHook {
private:
    PointShadowMappingStage& stage_;
    SharedStorageView<PointShadowMaps> stage_output_;
    GLsizei resolution_;

public:
    PointShadowMappingStageHook(PointShadowMappingStage& stage)
        : stage_{ stage }
        , stage_output_{ stage_.view_output() }
        , resolution_{ stage_output_->point_shadow_maps.width() }
    {}

    void operator()();
};


}
