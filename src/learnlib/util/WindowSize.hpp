#pragma once


namespace learn {


template<typename NumericT = int>
struct WindowSize {
    NumericT width;
    NumericT height;

    template<typename OtherT>
    operator WindowSize<OtherT>() const noexcept {
        return { static_cast<OtherT>(width), static_cast<OtherT>(height) };
    }

    template<typename FloatT = float>
    FloatT aspect_ratio() const noexcept {
        return static_cast<FloatT>(width) / static_cast<FloatT>(height);
    }
};


} // namespace learn
