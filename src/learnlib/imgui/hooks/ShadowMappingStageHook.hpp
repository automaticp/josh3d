#pragma once
#include "stages/ShadowMappingStage.hpp"
#include "SharedStorage.hpp"
#include "GLScalars.hpp"


namespace learn::imguihooks {


class ShadowMappingStageHook {
private:
    ShadowMappingStage& stage_;
    SharedStorageView<ShadowMappingStage::Output> shadow_info_;
    GLsizei point_shadow_res_;
    GLsizei dir_shadow_res_;

public:
    ShadowMappingStageHook(ShadowMappingStage& stage)
        : stage_{ stage }
        , shadow_info_{ stage_.view_mapping_output() }
        , point_shadow_res_{
            shadow_info_->point_light_maps.width()
        }
        , dir_shadow_res_{
            shadow_info_->dir_light_map.width()
        }
    {}

    void operator()();
};


} // namespace learn::imguihooks
