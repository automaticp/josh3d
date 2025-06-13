#pragma once
#include "AvgFrameTimeCounter.hpp"
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "GLObjects.hpp"
#include "RingBuffer.hpp"
#include "UniqueFunction.hpp"
#include <typeinfo>
#include <typeindex>
#include <concepts>
#include <chrono>


namespace josh {


class RenderEngine;
struct RenderEnginePrecomputeInterface;
struct RenderEnginePrimaryInterface;
struct RenderEnginePostprocessInterface;
struct RenderEngineOverlayInterface;

template<typename T> concept precompute_render_stage  = std::invocable<T, RenderEnginePrecomputeInterface&>;
template<typename T> concept primary_render_stage     = std::invocable<T, RenderEnginePrimaryInterface&>;
template<typename T> concept postprocess_render_stage = std::invocable<T, RenderEnginePostprocessInterface&>;
template<typename T> concept overlay_render_stage     = std::invocable<T, RenderEngineOverlayInterface&>;

using AnyPrecomputeStage  = UniqueFunction<void(RenderEnginePrecomputeInterface&)>;
using AnyPrimaryStage     = UniqueFunction<void(RenderEnginePrimaryInterface&)>;
using AnyPostprocessStage = UniqueFunction<void(RenderEnginePostprocessInterface&)>;
using AnyOverlayStage     = UniqueFunction<void(RenderEngineOverlayInterface&)>;

namespace detail {

class GPUTimer
{
public:
    auto get_timer() const noexcept -> const AvgFrameTimeCounter& { return counter_; }

    // Do this after querying the time interval.
    void emplace_new_time_query(UniqueQueryTimeElapsed query, float frame_time_delta_s)
    {
        time_queries_.emplace_front(MOVE(query), frame_time_delta_s);
    }

    void resolve_available_time_queries()
    {
        while (not time_queries_.is_empty() and
            time_queries_.back().query->is_available())
        {
            TimeQueryRequest query_request = time_queries_.pop_back();
            const auto slice_delta_s = std::chrono::duration<float>(query_request.query->result());
            counter_.update(slice_delta_s.count(), query_request.frame_time_delta_s);
        }
    }

    void set_averaging_interval(float interval_s) noexcept
    {
        counter_.averaging_interval = interval_s;
    }

private:
    struct TimeQueryRequest
    {
        UniqueQueryTimeElapsed query;
        float                  frame_time_delta_s;
    };

    AvgFrameTimeCounter             counter_{}; // Will need to preserve frame delta per query.
    BadRingBuffer<TimeQueryRequest> time_queries_{};
};

/*
A bunch of stuff about a stage thrown together for some basic
coherency. Private details are exposed to the RenderEngine.
Public interface is for the stage iterator interface.

Don't ask questions, I'm confused myself.
*/
template<typename AnyStageT>
struct Stage
{
public:
    // Something about exposing not only the public interface of the stage
    // but also it's lifetime and ownership really makes me wish for a nuclear winter.
    //
    // Do we need a FunctionRef?
    auto get()                    noexcept ->       AnyStageT&      { return stage_;           }
    auto get()              const noexcept -> const AnyStageT&      { return stage_;           }
    auto name()             const noexcept -> const String&         { return name_;            }
    auto stage_type()       const noexcept -> const std::type_info& { return *type_info_;      }
    auto stage_type_index() const noexcept ->       std::type_index { return { stage_type() }; }

    auto cpu_frametimer() const noexcept -> const AvgFrameTimeCounter& { return cpu_timer_; }
    auto gpu_frametimer() const noexcept -> const AvgFrameTimeCounter& { return gpu_timer_.get_timer(); }

private:
    friend RenderEngine;

    String                name_;
    AnyStageT             stage_;
    const std::type_info* type_info_;
    AvgFrameTimeCounter   cpu_timer_{};
    GPUTimer              gpu_timer_{}; // Will need to preserve frame delta per query.

    Stage(String name, AnyStageT&& stage)
        : name_     { MOVE(name)            }
        , stage_    { MOVE(stage)           }
        , type_info_{ &stage_.target_type() }
    {}
};

} // namespace detail

using PrecomputeStage  = detail::Stage<AnyPrecomputeStage>;
using PrimaryStage     = detail::Stage<AnyPrimaryStage>;
using PostprocessStage = detail::Stage<AnyPostprocessStage>;
using OverlayStage     = detail::Stage<AnyOverlayStage>;

} // namespace josh
