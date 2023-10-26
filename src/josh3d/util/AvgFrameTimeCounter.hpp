#pragma once
#include <numeric>
#include <vector>

namespace josh {


/*
Small helper for displaying average frametime/FPS
with a configurable averaging interval.
*/
class AvgFrameTimeCounter {
public:
    using seconds_t = float;
    seconds_t averaging_interval{ 0.200f };

private:
    seconds_t left_until_reset_{ averaging_interval };
    seconds_t current_average_frametime_{};
    seconds_t total_within_interval_{};
    size_t    num_frames_since_last_reset_{};

public:
    AvgFrameTimeCounter(seconds_t averaging_interval = 0.200f)
        : averaging_interval{ averaging_interval }
    {}

    // Call once every frame.
    void update(seconds_t delta_time) {

        ++num_frames_since_last_reset_;
        total_within_interval_ += delta_time;
        left_until_reset_      -= delta_time;
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

    // If the averaging_interval is updated, the last
    // average value will still be calculated with the
    // previous interval, before switching to the new one.
    // This will feel like 'lag' when changing the
    // interval from a big to a small one. This is fine.
    // I'm just too lazy to implement proper timer reset.
    seconds_t get_current_average() const noexcept {
        return current_average_frametime_;
    }

private:
    [[nodiscard]]
    seconds_t compute_average_and_reset() {
        seconds_t avg_frametime =
            total_within_interval_ / float(num_frames_since_last_reset_);

        total_within_interval_       = 0.f;
        num_frames_since_last_reset_ = 0;

        return avg_frametime;
    }

};

} // namespace josh
