#pragma once
#include "stages/primary/CascadedShadowMapping.hpp"


namespace josh::imguihooks::primary {


class CascadedShadowMapping {
private:
    stages::primary::CascadedShadowMapping& stage_;
    GLsizei resolution_;

public:
    CascadedShadowMapping(
        stages::primary::CascadedShadowMapping& stage)
        : stage_{ stage }
        , resolution_{ stage_.view_output().dir_shadow_maps_tgt.depth_attachment().size().width }
    {}

    void operator()();
};


} // namespace josh::imguihooks::primary
