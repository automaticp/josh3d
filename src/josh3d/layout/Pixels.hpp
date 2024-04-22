#pragma once
#include "Channels.hpp"
#include <utility>


namespace josh {


template<typename PixelT>
struct pixel_traits {
    using channel_type = decltype(std::declval<PixelT>().r);
    static constexpr size_t n_channels = sizeof(PixelT) / sizeof(channel_type);
    static constexpr bool   is_packed  = false;
};


namespace pixel {


struct RGBA {
    chan::UByte r, g, b, a;
};

struct RGB {
    chan::UByte r, g, b;
};

struct RG {
    chan::UByte r, g;
};

struct Red {
    chan::UByte r;
};

struct RGBAF {
    chan::UByte r, g, b, a;
};

struct RGBF {
    chan::Float r, g, b;
};

struct RGF {
    chan::Float r, g;
};

struct RedF {
    chan::Float r;
};


} // namespace pixel
} // namespace josh
