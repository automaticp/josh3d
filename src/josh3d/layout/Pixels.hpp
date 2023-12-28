#pragma once
#include <utility>


namespace josh {


using ubyte_t = unsigned char;


template<typename PixelT>
struct pixel_traits {
    using channel_type = decltype(std::declval<PixelT>().r);
    static constexpr size_t n_channels = sizeof(PixelT) / sizeof(channel_type);
};


namespace pixel {


struct RGBA {
    ubyte_t r, g, b, a;
};

struct RGB {
    ubyte_t r, g, b;
};

struct RG {
    ubyte_t r, g;
};

struct RED {
    ubyte_t r;
};

struct RGBAF {
    float r, g, b, a;
};

struct RGBF {
    float r, g, b;
};

struct RGF {
    float r, g;
};

struct REDF {
    float r;
};


} // namespace pixel
} // namespace josh
