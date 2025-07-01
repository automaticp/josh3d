#pragma once
#include "EnumUtils.hpp"
#include "KitchenSink.hpp"
#include "UniqueFunction.hpp"


namespace josh {

struct StageContext;
struct PrecomputeContext;
struct PrimaryContext;
struct PostprocessContext;
struct OverlayContext;

template<typename T> concept precompute_pipeline_stage  = std::invocable<T, PrecomputeContext>;
template<typename T> concept primary_pipeline_stage     = std::invocable<T, PrimaryContext>;
template<typename T> concept postprocess_pipeline_stage = std::invocable<T, PostprocessContext>;
template<typename T> concept overlay_pipeline_stage     = std::invocable<T, OverlayContext>;

enum class StageKind : u8
{
    Precompute,
    Primary,
    Postprocess,
    Overlay,
};
JOSH3D_DEFINE_ENUM_EXTRAS(StageKind, Precompute, Primary, Postprocess, Overlay);

/*
NOTE: Concrete context types (PrimaryContext, OverlayContext, etc.) are implicitly
convertible from StageContext since they add no member variables, so a single
"context-agnostic" definition is sufficent.
*/
JOSH3D_DERIVE_TYPE(PipelineStage, UniqueFunction<void(const StageContext&)>);


} // namespace josh
