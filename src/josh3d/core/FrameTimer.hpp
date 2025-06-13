#pragma once


namespace josh {


class FrameTimer
{
public:
    void update() noexcept;

    template<typename FloatT = double>
    auto current() const noexcept -> FloatT { return static_cast<FloatT>(current_); }
    template<typename FloatT = double>
    auto previous() const noexcept -> FloatT { return static_cast<FloatT>(previous_); }
    template<typename FloatT = double>
    auto delta() const noexcept -> FloatT { return static_cast<FloatT>(delta_); }

private:
    double current_{};
    double previous_{};
    double delta_{};
};

// EWW: There's very little reason to do this.
namespace globals {
inline FrameTimer frame_timer;
} // namespace globals

} // namespace josh
