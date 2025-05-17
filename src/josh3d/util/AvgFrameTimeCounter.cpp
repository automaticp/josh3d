#include "AvgFrameTimeCounter.hpp"


namespace josh {


void AvgFrameTimeCounter::update(seconds_t delta_time) {
    update(delta_time, delta_time);
}


void AvgFrameTimeCounter::update(
    seconds_t slice_delta_time,
    seconds_t total_delta_time)
{

    ++num_frames_since_last_reset_;

    total_within_interval_ += slice_delta_time;
    left_until_reset_      -= total_delta_time;

    if (left_until_reset_ < 0.0f) {

        current_average_frametime_ =
            compute_average_and_reset();

        // Subtract the time overflow from the next interval.
        // If the resulting interval is less than current frametime,
        // then just update average every frame (no average).
        left_until_reset_ = std::max(
            0.f,
            left_until_reset_ + averaging_interval
        );
    }
}


auto AvgFrameTimeCounter::compute_average_and_reset() noexcept
    -> seconds_t
{
    seconds_t avg_frametime =
        total_within_interval_ / float(num_frames_since_last_reset_);

    total_within_interval_       = 0.f;
    num_frames_since_last_reset_ = 0;

    return avg_frametime;
}


} // namespace josh
