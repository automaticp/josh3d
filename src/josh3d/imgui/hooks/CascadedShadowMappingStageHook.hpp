#pragma once
#include "SharedStorage.hpp"
#include "stages/CascadedShadowMappingStage.hpp"


namespace josh::imguihooks {


class CascadedShadowMappingStageHook {
private:
    CascadeViewsBuilder& builder_;
    CascadedShadowMappingStage& stage_;
    SharedStorageView<CascadedShadowMaps> stage_output_;
    GLsizei resolution_;

public:
    CascadedShadowMappingStageHook(
        CascadeViewsBuilder& cascade_builder,
        CascadedShadowMappingStage& stage)
        : builder_{ cascade_builder }
        , stage_{ stage }
        , stage_output_{ stage_.view_output() }
        , resolution_{ stage_output_->dir_shadow_maps_tgt.depth_attachment().size().width }
    {}

    void operator()();
};


} // namespace josh::imguihooks
