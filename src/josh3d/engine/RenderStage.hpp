#pragma once
#include "AvgFrameTimeCounter.hpp"
#include "GLObjects.hpp"
#include "RingBuffer.hpp"
#include "UniqueFunction.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>
#include <concepts>
#include <typeinfo>
#include <typeindex>


namespace josh {


class RenderEngine;
class RenderEnginePrecomputeInterface;
class RenderEnginePrimaryInterface;
class RenderEnginePostprocessInterface;
class RenderEngineOverlayInterface;


template<typename StageT>
concept precompute_render_stage =
    std::invocable<StageT, const RenderEnginePrecomputeInterface&, entt::registry&>;

template<typename StageT>
concept primary_render_stage =
    std::invocable<StageT, const RenderEnginePrimaryInterface&, const entt::registry&>;

template<typename StageT>
concept postprocess_render_stage =
    std::invocable<StageT, const RenderEnginePostprocessInterface&, const entt::registry&>;

template<typename StageT>
concept overlay_render_stage =
    std::invocable<StageT, const RenderEngineOverlayInterface&, const entt::registry&>;




/*
Type erased precompute stage stored inside the RenderEngine stages container.
*/
using AnyPrecomputeStage =
    UniqueFunction<void(const RenderEnginePrecomputeInterface&, entt::registry&)>;

/*
Type erased primary stage stored inside the RenderEngine stages container.
*/
using AnyPrimaryStage =
    UniqueFunction<void(const RenderEnginePrimaryInterface&, const entt::registry&)>;

/*
Type erased postfx stage stored inside the RenderEngine stages container.
*/
using AnyPostprocessStage =
    UniqueFunction<void(const RenderEnginePostprocessInterface&, const entt::registry&)>;

/*
Type erased overlay stage stored inside the RenderEngine stages container.
*/
using AnyOverlayStage =
    UniqueFunction<void(const RenderEngineOverlayInterface&, const entt::registry&)>;




namespace detail {


/*
A bunch of stuff about a stage thrown together for some basic
coherency. Private details are exposed to the RenderEngine.
Public interface is for the stage iterator interface.

Don't ask questions, I'm confused myself.
*/
template<typename AnyStageT>
class Stage {
private:
    friend RenderEngine;

    std::string           name_;
    AnyStageT             stage_;
    const std::type_info* type_info_;
    AvgFrameTimeCounter   cpu_timer_{};
    AvgFrameTimeCounter   gpu_timer_{}; // Will need to preserve frame delta per query.

    // TODO: Probably encapsulate this "GPU Timer" stuff into a type.
    struct TimeQueryRequest {
        UniqueTimerQuery query;
        float frame_time_delta_s;
    };
    BadRingBuffer<TimeQueryRequest> time_queries_;

    // Do this after querying the time interval.
    void emplace_new_time_query(UniqueTimerQuery query, float frame_time_delta_s) {
        time_queries_.emplace_front(std::move(query), frame_time_delta_s);
    }

    void resolve_available_time_queries() {
        while (!time_queries_.is_empty() &&
            time_queries_.back().query.is_available())
        {
            TimeQueryRequest query_request = time_queries_.pop_back();
            auto slice_delta_s = std::chrono::duration<float>(query_request.query.result());
            gpu_timer_.update(slice_delta_s.count(), query_request.frame_time_delta_s);
        }
    }


    Stage(std::string name, AnyStageT&& stage)
        : name_{ std::move(name) }
        , stage_{ std::move(stage) }
        , type_info_{ &stage_.target_type() }
    {}

public:
    // Something about exposing not only the public interface of the stage
    // but also it's lifetime and ownership really makes me wish for a nuclear winter.
    //
    // Do we need a FunctionRef?
          AnyStageT&      get()              noexcept { return stage_;      }
    const AnyStageT&      get()        const noexcept { return stage_;      }
    const std::string&    name()       const noexcept { return name_;       }
    const std::type_info& stage_type() const noexcept { return *type_info_; }
          std::type_index stage_type_index() const noexcept { return { stage_type() }; }

    const AvgFrameTimeCounter& cpu_frametimer() const noexcept { return cpu_timer_; }
    const AvgFrameTimeCounter& gpu_frametimer() const noexcept { return gpu_timer_; }

};


} // namespace detail




using PrecomputeStage  = detail::Stage<AnyPrecomputeStage>;
using PrimaryStage     = detail::Stage<AnyPrimaryStage>;
using PostprocessStage = detail::Stage<AnyPostprocessStage>;
using OverlayStage     = detail::Stage<AnyOverlayStage>;




} // namespace josh
