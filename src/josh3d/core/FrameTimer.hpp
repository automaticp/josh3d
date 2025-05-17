#pragma once


namespace josh {


class FrameTimer {
private:
    double current_{};
    double previous_{};
    double delta_{};

public:
    void update() noexcept;

    template<typename FloatT = double>
    FloatT current() const noexcept { return static_cast<FloatT>(current_); }
    template<typename FloatT = double>
    FloatT previous() const noexcept { return static_cast<FloatT>(previous_); }
    template<typename FloatT = double>
    FloatT delta() const noexcept { return static_cast<FloatT>(delta_); }
};


namespace globals {
inline FrameTimer frame_timer;
} // namespace globals


} // namespace josh
