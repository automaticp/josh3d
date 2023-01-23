#pragma once
#include <array>
#include <string_view>
#include <utility>
#include "TextureData.hpp"

namespace learn {


class CubemapData {
private:
    // Target order:
    // +X, -X, +Y, -Y, +Z, -Z
    // Array index corresponds to the
    // offset from GL_TEXTURE_CUBE_MAP_POSITIVE_X
    std::array<TextureData, 6> sides_;

public:
    CubemapData(std::array<TextureData, 6> sides)
        : sides_{ std::move(sides) }
    {}

    static CubemapData from_files(std::array<const char*, 6> filenames) {
        std::array<TextureData, 6> sides{
            TextureData(StbImageData(filenames[0], false)),
            TextureData(StbImageData(filenames[1], false)),
            TextureData(StbImageData(filenames[2], false)),
            TextureData(StbImageData(filenames[3], false)),
            TextureData(StbImageData(filenames[4], false)),
            TextureData(StbImageData(filenames[5], false))
        };
        return CubemapData{ std::move(sides) };
    }

    std::array<TextureData, 6>& data() noexcept { return sides_; }
    const std::array<TextureData, 6>& data() const noexcept { return sides_; }

};



} //namespace learn
