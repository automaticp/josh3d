#pragma once
#include "NumericLimits.hpp"
#include "Scalars.hpp"
#include "StaticRing.hpp"
#include "Time.hpp"


namespace josh {


/*
Double-buffered timing helper that counts average, min and max
durations over user-controlled flush intervals.
*/
struct AggregateTimer
{
    // Record a new time delta, updating the min/max and the mean counts.
    void record(TimeDeltaNS dt);

    // Flushes the uncomputed total averages and swaps states to present.
    void flush();

    // Reset next state, effectively setting all values to 0.
    // The current state is still reported until the next flush.
    void reset();

    struct State
    {
        TimeDeltaNS mean  = {};
        TimeDeltaNS min   = TimeDeltaNS(vmax<TimeDeltaNS::rep>);
        TimeDeltaNS max   = TimeDeltaNS(vmin<TimeDeltaNS::rep>);
    };

    // Returns the current state, recorded before the last flush.
    auto current() const noexcept -> const State& { return _states.current(); }

    // The timer exists in 2 states, the current is what's being displayed.
    StaticRing<State, 2> _states = {};

    // This accumulates deltas until the mean needs to be flushed.
    TimeDeltaNS _total = {};
    usize       _count = {};
};


inline void AggregateTimer::record(TimeDeltaNS dt)
{
    auto& s = _states.next();
    s.min = std::min(s.min, dt);
    s.max = std::max(s.max, dt);

    // NOTE: Not updating s.mean until the flush. Also div is expensive.
    _total += dt;
    ++_count;
}

inline void AggregateTimer::flush()
{
    _states.advance();
    if (_count) _states.current().mean = _total / _count;
    else        _states.current().mean = TimeDeltaNS::zero();
    reset();
}

inline void AggregateTimer::reset()
{
    _total = {};
    _count = {};
    _states.next() = {};
}


} // namespace josh
