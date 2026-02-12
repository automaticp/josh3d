#pragma once
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "PerfHarness.hpp"
#include "SystemKey.hpp"
#include "Time.hpp"


namespace josh {


/*
A collection of PerfHarnesses for tracking system/stage performance globally.

TODO: There should be an option to pause measurements.
*/
struct PerfAssembly
{
    // The mean values of all timers will be flushed at this rate.
    // Note that the GPU timing is asyncronous and might lag behind by a frame or two.
    TimeDeltaNS flush_interval = TimeDeltaNS::from_seconds(0.5);

    auto instrument(SystemKey key, GPUTiming gpu_timing = GPUTiming::Disabled) -> PerfHarness&;
    auto try_get(SystemKey key)       -> PerfHarness*;
    auto try_get(SystemKey key) const -> const PerfHarness*;
    void collect_all(TimeDeltaNS frame_dt);

    HashMap<SystemKey, PerfHarness> _harnesses;
    TimeDeltaNS                     _until_next_flush = flush_interval;
};


inline auto PerfAssembly::instrument(SystemKey key, GPUTiming gpu_timing)
    -> PerfHarness&
{
    auto [it, was_emplaced] = _harnesses.try_emplace(key, gpu_timing);
    auto& harness = it->second;

    // NOTE: In case the above did not emplace, we would like
    // to override the GPU timing state anyways.
    if (not was_emplaced)
        harness.set_gpu_timing(bool(gpu_timing));

    return it->second;
}

inline auto PerfAssembly::try_get(SystemKey key)
    -> PerfHarness*
{
    return try_find_value(_harnesses, key);
}

inline auto PerfAssembly::try_get(SystemKey key) const
    -> const PerfHarness*
{
    return try_find_value(_harnesses, key);
}

inline void PerfAssembly::collect_all(TimeDeltaNS frame_dt)
{
    bool needs_flush = false;
    _until_next_flush -= frame_dt;

    if (_until_next_flush < TimeDeltaNS::zero())
    {
        needs_flush = true;

        // Subtract the time overflow from the next interval.
        //
        // If the resulting interval ends up being less than current frametime,
        // then we'll just flush means every frame (effectively no mean).
        _until_next_flush = std::max(TimeDeltaNS::zero(), _until_next_flush + flush_interval);
    }

    for (auto& [key, harness] : _harnesses)
    {
        // TODO: Should be checked if we taken any snaps this frame.
        harness.collect_frame();

        // HMM: Should we account for GPU latency when flushing?
        // That is, should we flush the GPU data 2-3 frames later? Who cares?
        if (needs_flush)
            for (auto& [segment_id, segment] : harness._segments)
                segment.flush_all_timers();
    }
}


} // namespace josh
