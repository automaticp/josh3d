#pragma once
#include "ImageData.hpp"
#include "PixelData.hpp"
#include <array>
#include <utility>


namespace josh {


template<typename PixelT>
class CubemapPixelData {
public:
    CubemapPixelData(std::array<PixelData<PixelT>, 6> sides)
        : sides_{ std::move(sides) }
    {}

    CubemapPixelData(
        PixelData<PixelT> posx, PixelData<PixelT> negx,
        PixelData<PixelT> posy, PixelData<PixelT> negy,
        PixelData<PixelT> posz, PixelData<PixelT> negz)
        : sides_{
            std::move(posx), std::move(negx),
            std::move(posy), std::move(negy),
            std::move(posz), std::move(negz)
        }
    {}

          std::array<PixelData<PixelT>, 6>& sides()       noexcept { return sides_; }
    const std::array<PixelData<PixelT>, 6>& sides() const noexcept { return sides_; }

private:
    // Target order:
    // +X, -X, +Y, -Y, +Z, -Z
    // Array index corresponds to the
    // offset from GL_TEXTURE_CUBE_MAP_POSITIVE_X
    std::array<PixelData<PixelT>, 6> sides_;

};




template<typename ChannelT>
class CubemapImageData {
public:
    using channel_type = ChannelT;
    using image_type   = ImageData<ChannelT>;

    CubemapImageData(std::array<ImageData<ChannelT>, 6> sides)
        : sides_{ std::move(sides) }
    {}

    CubemapImageData(
        ImageData<ChannelT> posx, ImageData<ChannelT> negx,
        ImageData<ChannelT> posy, ImageData<ChannelT> negy,
        ImageData<ChannelT> posz, ImageData<ChannelT> negz)
        : sides_{
            std::move(posx), std::move(negx),
            std::move(posy), std::move(negy),
            std::move(posz), std::move(negz)
        }
    {}

          std::array<ImageData<ChannelT>, 6>& sides()       noexcept { return sides_; }
    const std::array<ImageData<ChannelT>, 6>& sides() const noexcept { return sides_; }

private:
    // Target order:
    // +X, -X, +Y, -Y, +Z, -Z
    // Array index corresponds to the
    // offset from GL_TEXTURE_CUBE_MAP_POSITIVE_X
    std::array<ImageData<ChannelT>, 6> sides_;
};


} //namespace josh
