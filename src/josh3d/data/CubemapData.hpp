#pragma once
#include "ImageData.hpp"
#include <array>
#include <utility>


namespace josh {


template<typename PixelT>
class CubemapData {
private:
    // Target order:
    // +X, -X, +Y, -Y, +Z, -Z
    // Array index corresponds to the
    // offset from GL_TEXTURE_CUBE_MAP_POSITIVE_X
    std::array<ImageData<PixelT>, 6> sides_;

public:
    CubemapData(std::array<ImageData<PixelT>, 6> sides)
        : sides_{ std::move(sides) }
    {}

    CubemapData(
        ImageData<PixelT> posx, ImageData<PixelT> negx,
        ImageData<PixelT> posy, ImageData<PixelT> negy,
        ImageData<PixelT> posz, ImageData<PixelT> negz)
        : sides_{
            std::move(posx), std::move(negx),
            std::move(posy), std::move(negy),
            std::move(posz), std::move(negz)
        }
    {}

          std::array<ImageData<PixelT>, 6>& sides()       noexcept { return sides_; }
    const std::array<ImageData<PixelT>, 6>& sides() const noexcept { return sides_; }

};


} //namespace josh
