#pragma once
#include "CommonConcepts.hpp"
#include "KitchenSink.hpp"
#include <chrono>
#include <concepts>


/*
Defines the common vocabulary for representing time.
*/
namespace josh {


/*
Canonical duration (time delta) represented as the number of nanoseconds.

Note that while the "representation granularity" is in nanoseconds,
the *precision* of values themselves or the resolution of the clocks
returning them is not required to be that high.
*/
JOSH3D_DERIVE_TYPE_EX(TimeDeltaNS, std::chrono::nanoseconds)
//{
    // Convert a time delta to the value of ReprT type.
    // The conversion is possibly lossy.
    template<std::floating_point ReprT>
    auto to_seconds() const noexcept
        -> ReprT
    {
        return duration_cast<std::chrono::duration<ReprT>>(*this).count();
    }

    // Create a nanosecond time delta from a count of seconds.
    // The conversion is possibly lossy.
    template<arithmetic T>
    static auto from_seconds(T seconds) noexcept
        -> TimeDeltaNS
    {
        return { duration_cast<std::chrono::nanoseconds>(std::chrono::duration<T>(seconds)) };
    }
};

JOSH3D_DERIVE_TYPE(TimePointNS, std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>);

inline auto current_time() noexcept
    -> TimePointNS
{
    return TimePointNS(std::chrono::high_resolution_clock::now());
}

namespace detail {
struct opaque_clock {};
} // namespace detail

/*
A TimeStamp is a lot like TimePoint except the clock that it is taken from
is considered "opaque" or "implied". It exists to derive deltas/durations
or cooperate with stateful clocks.

NOTE: This is technically not legal before C++23 but because the requirements
for `Clock` are not checked anywhere, this works just fine. See: https://wg21.link/P2212

NOTE: TimeStamps must be explicitly constructed from the corresponding duration type.
*/
JOSH3D_DERIVE_TYPE(TimeStampNS, std::chrono::time_point<detail::opaque_clock, std::chrono::nanoseconds>);


} // namespace josh
