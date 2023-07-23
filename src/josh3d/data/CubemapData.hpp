#pragma once
#include <array>
#include <string_view>
#include <utility>
#include "Filesystem.hpp"
#include "TextureData.hpp"

namespace josh {


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

    CubemapData(
        TextureData posx, TextureData negx,
        TextureData posy, TextureData negy,
        TextureData posz, TextureData negz)
        : sides_{
            std::move(posx), std::move(negx),
            std::move(posy), std::move(negy),
            std::move(posz), std::move(negz)
        }
    {}

    static CubemapData from_files(const File (&files)[6]) {
        std::array<TextureData, 6> sides{
            TextureData::from_file(files[0], false),
            TextureData::from_file(files[1], false),
            TextureData::from_file(files[2], false),
            TextureData::from_file(files[3], false),
            TextureData::from_file(files[4], false),
            TextureData::from_file(files[5], false)
        };
        return CubemapData{ std::move(sides) };
    }


    [[nodiscard]]
    static CubemapData from_files(
        const File& posx, const File& negx,
        const File& posy, const File& negy,
        const File& posz, const File& negz)
    {
        return CubemapData {
            TextureData::from_file(posx, false),
            TextureData::from_file(negx, false),
            TextureData::from_file(posy, false),
            TextureData::from_file(negy, false),
            TextureData::from_file(posz, false),
            TextureData::from_file(negz, false)
        };
    }

    std::array<TextureData, 6>& data() noexcept { return sides_; }
    const std::array<TextureData, 6>& data() const noexcept { return sides_; }

};



} //namespace josh
