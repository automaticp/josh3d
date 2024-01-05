#pragma once
#include "SharedStorage.hpp"
#include "stages/primary/CascadedShadowMapping.hpp"


namespace josh::imguihooks::primary {


class CascadedShadowMapping {
private:
    CascadeViewsBuilder& builder_;
    stages::primary::CascadedShadowMapping& stage_;
    SharedStorageView<CascadedShadowMaps> stage_output_;
    GLsizei resolution_;

public:
    CascadedShadowMapping(
        CascadeViewsBuilder& cascade_builder,
        stages::primary::CascadedShadowMapping& stage)
        : builder_{ cascade_builder }
        , stage_{ stage }
        , stage_output_{ stage_.view_output() }
        , resolution_{ stage_output_->dir_shadow_maps_tgt.depth_attachment().size().width }
    {}

    void operator()();
};


} // namespace josh::imguihooks::primary
