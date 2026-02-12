#pragma once
#include "ContainerUtils.hpp"
#include "AggregateTimer.hpp"
#include "EnumUtils.hpp"
#include "GLAPICore.hpp"
#include "GLObjects.hpp"
#include "HashedString.hpp"
#include "Logging.hpp"
#include "Ranges.hpp"
#include "StaticRing.hpp"
#include "Time.hpp"
#include <ranges>


namespace josh {


enum class GPUTiming : bool
{
    Disabled = false,
    Enabled  = true,
};
JOSH3D_DEFINE_ENUM_EXTRAS(GPUTiming, Disabled, Enabled);


/*
A performance measurement "wrapper" around a coarse unit of work: system, stage, etc.

TODO: We should probably provide an interface for explicit segments by using
either manual start/end segment calls, and/or the classic RAII-based mechanisms.
*/
struct PerfHarness
{
    struct Frame;
    struct Segment;

    PerfHarness(GPUTiming gpu_timing = GPUTiming::Disabled) : _time_gpu(bool(gpu_timing)) {}

    // Begin a new frame and take the corresponding "start" snap.
    void start_frame();

    // Take an intermediate snapshot within the frame with a custom name.
    // The name can be anything other that the reserved "start" and "end" identifiers.
    template<usize N> requires (N > 0)
    void take_snap(FixedHashedString<N> name_hs);

    // End recording of the frame by taking the "end" snap.
    void end_frame();

    // Take the available snapshot data from the last frames and
    // use it to update the AggregateTimers of each respective segment.
    //
    // PRE: Must be called after `end_frame()`.
    void collect_frame();

    // Controls whether GPU timing will be performed by the harness.
    // Will only take effert on the next frame. This is all-or-nothing,
    // expect wild values for the next 3 frames or so after changing this
    // value. Also, when true, all snaps must be taken in a valid GPU context.
    void set_gpu_timing(bool enabled);
    auto is_gpu_timed() const noexcept -> bool { return _time_gpu; }

    // Returns a const view over key-value pairs of recorded segments.
    // The segment with HashedID of 0 represents the "full" segment
    // that covers the entire frame from "start" to "end".
    // The segments are not ordered, use last_frame() to infer
    // order from taken snapshots.
    auto view_segments() const noexcept -> std::ranges::view auto;

    // Returns a segment for a particular id, if present.
    // If the id exists in `last_frame().snaps`, it should
    // have an associated segment.
    //
    // PRE: Must be called after `collect_frame()`.
    auto get_segment(HashedID id) const noexcept -> const Segment*;

    // Returns the view of the last frame.
    //
    // PRE: Must be called after `collect_frame()`.
    auto last_frame() const noexcept -> const Frame&;


    struct CPUSnap
    {
        TimePointNS wall_time; // CPU time measured by a wall-clock.
    };

    struct GPUSnap
    {
        TimeStampNS host_time; // GL server time taken from glGetInteger(GL_TIMESTAMP, ...).

        // Whether device_time is available. Will not change state until try_resolve_query() succeeds.
        auto device_time_available() const noexcept -> bool { return is<TimeStampNS>(_device_time); }
        // Returns true if the resolve succeded and the device time became available.
        auto try_resolve_query() noexcept -> bool;
        // Returns device time if it is availabe or a null timestamp otherwise.
        auto device_time() const noexcept -> TimeStampNS;
        // Latency between host_time and device_time. Only available together with device_time.
        auto device_latency() const noexcept -> TimeDeltaNS;

        explicit operator bool() const noexcept { return not is<None>(_device_time); }

        struct None {}; // Tag to detect when no device query was made.
        // Device time measured as an async query. The current alternative dictates availability state.
        Variant<None, UniqueQueryTimestamp, TimeStampNS> _device_time;
    };

    struct Snap
    {
        HashedID id;  // A snapshot identifier unique per-Frame. Corresponds to its segment.
        CPUSnap  cpu; //
        GPUSnap  gpu; // Might not have data. HMM: This bloats the structure sizes.
    };

    /*
    A frame is a series of snapshots between "start" and "end".
    This is unrelated to the concept of "rendering frame" and only
    covers the operation that the harness directly wraps.
    */
    struct Frame
    {
        // There shoud be at least 2 snaphots taken per-harness per-frame:
        // the start and the end shapshots. But more can be taken between those.
        SmallVector<Snap, 2> snaps;
    };

    /*
    A segment represents a pair of snapshots in a frame.
    */
    struct Segment
    {
        String         name;
        AggregateTimer wall_time   = {};
        AggregateTimer host_time   = {}; // Not very useful.
        AggregateTimer device_time = {};
        AggregateTimer latency     = {}; // HMM: Is this a timer?
        void flush_all_timers();
        void reset_all_timers();
    };

    // TODO: We should gracefully handle cases where no snaps are taken during a frame.
    // Currently, we'll likely fall over and die in collect_frame().
    // Do we need a new call like `new_frame()`?

    bool                       _time_gpu;                // Whether the GPU needs to be timed this frame.
    bool                       _in_frame        = false; // [DEBUG] Tracks if we are within a "frame" or not - start_frame() has been called.
    bool                       _frame_ended     = false; // [DEBUG] Tracks if the end_frame() has been called.
    bool                       _frame_collected = false; // [DEBUG] Tracks if the current frame has been flushed already.

    // Ring buffer for last 3 frames. This should be enough to resolve queries.
    StaticRing<Frame, 3>       _frames;
    // Per-segment info. Segments are non-overlapping except for the special "full" segment with ID of 0.
    HashMap<HashedID, Segment> _segments = { { 0, { "full" } } };

    // This will only emplace a new entry on first encounter of the name.
    // Then we will just keep the association until the harness is destroyed.
    //
    // Will reset all AggregateTimers if a new segment was created because
    // this changes the overall "segmentation" of a frame.
    auto _push_segment(HashedID id, const char* name) -> Segment&;
};


inline void PerfHarness::start_frame()
{
    assert(not _in_frame);
    _frames.advance();
    _frames.current().snaps.clear();
    _frame_ended     = false;
    _frame_collected = false;
    _in_frame        = true;
    take_snap("start"_hs);
}

inline void PerfHarness::end_frame()
{
    assert(_in_frame);
    assert(not _frame_ended);
    take_snap("end"_hs);
    assert(_frames.current().snaps.size() >= 2);
    _in_frame    = false;
    _frame_ended = true;
}

template<usize N> requires (N > 0)
void PerfHarness::take_snap(FixedHashedString<N> name_hs)
{
    assert(_in_frame);

    _push_segment(name_hs.hash(), name_hs.c_str());

    CPUSnap cpu_snap = {
        .wall_time = current_time(),
    };

    GPUSnap gpu_snap = eval%[&]() -> GPUSnap {
        if (_time_gpu)
        {
            const auto host_time = TimeStampNS(glapi::get_current_time());

            UniqueQueryTimestamp query;
            query->record_time();

            return {
                .host_time    = host_time,
                ._device_time = MOVE(query)
            };
        }
        return {};
    };

    _frames.current().snaps.push_back({
        .id  = name_hs.hash(),
        .cpu = cpu_snap,
        .gpu = MOVE(gpu_snap),
    });
}

inline void PerfHarness::collect_frame()
{
    assert(not _in_frame);
    assert(_frame_ended);
    assert(not _frame_collected);

    const auto for_each_segment = [this](Span<Snap> snaps, auto&& f)
    {
        const usize num_snaps = snaps.size();
        if (not num_snaps) return;

        // This is the most "C++20 ranges" moment ever.
        auto v1 = irange(num_snaps - 1);
        auto v2 = irange(1, num_snaps);

        for (const auto [ithis, inext] : zip(v1, v2))
        {
            // The time from "this" to "next" defines each segment.
            Snap&    this_snap    = snaps[ithis];
            Snap&    next_snap    = snaps[inext];
            Segment& this_segment = _segments.at(this_snap.id);

            f(this_segment, this_snap, next_snap);
        }
    };

    const auto record_cpu = [](Segment& s, Snap& l, Snap& r)
    {
        s.wall_time.record(r.cpu.wall_time - l.cpu.wall_time);
    };

    const auto record_gpu = [](Segment& s, Snap& l, Snap& r)
    {
        if (not l.gpu or not r.gpu)
            return;

        const bool both_resolved =
            l.gpu.try_resolve_query() and
            r.gpu.try_resolve_query();

        if (not both_resolved)
        {
            logstream() << "WARNING: GPU timestamp query dropped. Increase ring buffer size.\n";
            return;
        }

        s.host_time  .record(r.gpu.host_time        - l.gpu.host_time);
        s.device_time.record(r.gpu.device_time()    - l.gpu.device_time());
        s.latency    .record(r.gpu.device_latency());
    };

    // The "full" segment is special and is assigned a hash id of 0.
    // This segment represets the entire frame from "start" to "end"
    // and is generally useful to have available.
    //
    // The full segment is not stored in the segment lists, and
    // is recorded *in addition* to normal "adjacent" segments.
    auto& full_segment = _push_segment(0, "full");

    // CPU measurements are taken directly from the last (head) frame.
    auto& head_snaps = _frames.current().snaps;
    for_each_segment(head_snaps, record_cpu);
    record_cpu(full_segment, head_snaps.front(), head_snaps.back());

    // In the GPU case, we only collect once the full segment has been
    // recorded on the device. Otherwise, going back and trying to untangle
    // which segments already have host time collected, but not device time
    // vs which segments have neither vs which have both is a PITA.

    // The GPU capture is always lagging behind. We just record with a steady
    // latency of 2 frames - next() points directly to the tail of the ring buffer.
    auto& tail_snaps = _frames.next().snaps;
    if (not tail_snaps.empty()) // Could be empty for the first few frames.
    {
        for_each_segment(tail_snaps, record_gpu);
        record_gpu(full_segment, tail_snaps.front(), tail_snaps.back());
    }

    // HMM: Since the GPU timers are async, it *might* make sense to let us repeatedly
    // collect the frame until all queries have been resolved. Is that useful?
    _frame_collected = true;
}

inline void PerfHarness::set_gpu_timing(bool enabled)
{
    assert(not _in_frame);
    if (enabled != is_gpu_timed())
    {
        // HMM: Do we need to flush or reset anything?
        _time_gpu = enabled;
    }
}

inline auto PerfHarness::view_segments() const noexcept
    -> std::ranges::view auto
{
    return std::views::all(_segments);
}

inline auto PerfHarness::get_segment(HashedID id) const noexcept
    -> const Segment*
{
    assert(_frame_collected);
    return try_find_value(_segments, id);
}

inline auto PerfHarness::last_frame() const noexcept
    -> const Frame&
{
    assert(_frame_collected);
    return _frames.current();
}

inline auto PerfHarness::_push_segment(HashedID id, const char* name)
    -> Segment&
{
    auto [it, was_emplaced] = _segments.try_emplace(id, name);

    if (was_emplaced)
        for (auto& [id, segment] : _segments)
            segment.reset_all_timers();

    return it->second;
}

inline auto PerfHarness::GPUSnap::try_resolve_query() noexcept
    -> bool
{
    if (device_time_available())
        return true;

    auto& query = get<UniqueQueryTimestamp>(_device_time);
    if (query->is_available())
    {
        _device_time = TimeStampNS(query->result());
        return true;
    }

    return false;
}

inline auto PerfHarness::GPUSnap::device_time() const noexcept
    -> TimeStampNS
{
    if (device_time_available())
        return get<TimeStampNS>(_device_time);
    return {};
}

inline auto PerfHarness::GPUSnap::device_latency() const noexcept
    -> TimeDeltaNS
{
    if (not device_time_available())
        return {};
    return device_time() - host_time;
}

inline void PerfHarness::Segment::flush_all_timers()
{
    wall_time  .flush();
    host_time  .flush();
    device_time.flush();
    latency    .flush();
}

inline void PerfHarness::Segment::reset_all_timers()
{
    wall_time  .reset();
    host_time  .reset();
    device_time.reset();
    latency    .reset();
}


} // namespace josh
