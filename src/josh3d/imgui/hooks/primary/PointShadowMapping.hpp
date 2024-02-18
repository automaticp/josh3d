#pragma once
#include "SharedStorage.hpp"
#include "GLScalars.hpp"
#include "stages/primary/PointShadowMapping.hpp"


namespace josh::imguihooks::primary {


class PointShadowMapping {
private:
    stages::primary::PointShadowMapping& stage_;
    GLsizei resolution_;

public:
    PointShadowMapping(stages::primary::PointShadowMapping& stage)
        : stage_{ stage }
        , resolution_{ stage.view_output().point_shadow_maps_tgt.depth_attachment().size().width }
    {}

    void operator()();
};


} // namespace josh::imguihooks::primary
