#pragma once
#include "stages/CascadedShadowMappingStage.hpp"


namespace josh::imguihooks {


class CascadedShadowMappingStageHook {
private:
    CascadeViewsBuilder& builder_;
    CascadedShadowMappingStage& stage_;

public:
    CascadedShadowMappingStageHook(
        CascadeViewsBuilder& cascade_builder,
        CascadedShadowMappingStage& stage)
        : builder_{ cascade_builder }
        , stage_{ stage }
    {}

    void operator()();
};


} // namespace josh::imguihooks
