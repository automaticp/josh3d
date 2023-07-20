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
    seconds_t current_frametime_until_reset_{ averaging_interval };
    seconds_t current_average_frametime_{};
    std::vector<seconds_t> last_frametimes_;

public:
    AvgFrameTimeCounter(seconds_t averaging_interval = 0.200f)
        : averaging_interval{ averaging_interval }
    {}

    // Call once every frame.
    void update(seconds_t delta_time) {

        last_frametimes_.push_back(delta_time);
        current_frametime_until_reset_ -= delta_time;

        if (current_frametime_until_reset_ < 0.0f) {

            current_average_frametime_ =
                compute_average_from_buffer_and_reset();

            // Subtract the time overflow from the next interval.
            // If the resulting interval is less than current frametime,
            // then just update average every frame (no average).
            current_frametime_until_reset_ = std::max(
                0.f,
                current_frametime_until_reset_ + averaging_interval
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
    seconds_t compute_average_from_buffer_and_reset() {
        seconds_t avg_frametime =
            std::reduce(
                last_frametimes_.begin(), last_frametimes_.end()
            )
            / static_cast<float>(last_frametimes_.size());

        last_frametimes_.clear();

        return avg_frametime;
    }

};

} // namespace josh
