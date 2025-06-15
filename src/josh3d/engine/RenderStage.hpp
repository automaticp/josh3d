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


struct RenderEngine;
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
    auto get()                    noexcept ->       AnyStageT&      { return _stage;           }
    auto get()              const noexcept -> const AnyStageT&      { return _stage;           }
    auto name()             const noexcept -> const String&         { return _name;            }
    auto stage_type()       const noexcept -> const std::type_info& { return *_type_info;      }
    auto stage_type_index() const noexcept ->       std::type_index { return { stage_type() }; }
    auto cpu_frametimer()   const noexcept -> const AvgFrameTimeCounter& { return _cpu_timer; }
    auto gpu_frametimer()   const noexcept -> const AvgFrameTimeCounter& { return _gpu_timer.get_timer(); }

    String                _name;
    AnyStageT             _stage;
    const std::type_info* _type_info;
    AvgFrameTimeCounter   _cpu_timer = {};
    GPUTimer              _gpu_timer = {}; // Will need to preserve frame delta per query.

    Stage(String name, AnyStageT&& stage)
        : _name     { MOVE(name)            }
        , _stage    { MOVE(stage)           }
        , _type_info{ &_stage.target_type() }
    {}
};

} // namespace detail

using PrecomputeStage  = detail::Stage<AnyPrecomputeStage>;
using PrimaryStage     = detail::Stage<AnyPrimaryStage>;
using PostprocessStage = detail::Stage<AnyPostprocessStage>;
using OverlayStage     = detail::Stage<AnyOverlayStage>;

} // namespace josh
