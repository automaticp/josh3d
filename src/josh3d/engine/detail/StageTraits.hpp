#pragma once
#include "RenderStage.hpp"


namespace josh::detail {


enum class StageType {
    // For use in macros, it is convinient for the enum to have
    // the same member names as stage namespace names.
    precompute,
    primary,
    postprocess,
    overlay
};




template<StageType> struct stage_traits;

template<> struct stage_traits<StageType::precompute> {
    using any_type = AnyPrecomputeStage;
};

template<> struct stage_traits<StageType::primary> {
    using any_type = AnyPrimaryStage;
};

template<> struct stage_traits<StageType::postprocess> {
    using any_type = AnyPostprocessStage;
};

template<> struct stage_traits<StageType::overlay> {
    using any_type = AnyOverlayStage;
};




} // namespace josh::detail
