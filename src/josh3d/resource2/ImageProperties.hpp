#pragma once
#include "ContainerUtils.hpp"
#include "EnumUtils.hpp"
#include "GLTextures.hpp"
#include "Scalars.hpp"


/*
Kitchen-sink vocabulary for describing images and textures.
*/
namespace josh {

enum class Colorspace : u8
{
    Linear,
    sRGB,
};
JOSH3D_DEFINE_ENUM_EXTRAS(Colorspace, Linear, sRGB);

/*
NOTE: This is supposed to reflect the serialized format of TextureFile,
although the exact underlying type or values are not guaranteed.
*/
enum class ImageEncoding : u8
{
    Raw, // No compression. Directly streamable.
    PNG, // High compression. Needs decoding.
    BC7, // Low compression. Directly streamable.
};
JOSH3D_DEFINE_ENUM_EXTRAS(ImageEncoding, Raw, PNG, BC7);

/*
R8/RG8/RGB8/RGBA8 from num_channels.
PRE: num_channels: [1, 4].
*/
inline auto ubyte_iformat_from_num_channels(usize num_channels)
    -> InternalFormat
{
    switch (num_channels)
    {
        case 1: return InternalFormat::R8;
        case 2: return InternalFormat::RG8;
        case 3: return InternalFormat::RGB8;
        case 4: return InternalFormat::RGBA8;
        default: panic_fmt("Invalid number of image channels: {}.", num_channels);
    }
}

/*
RGB8/RGBA8 or sRGB8/sRGBA8 based on the colorspace.
PRE: num_channels: [3, 4]
*/
inline auto ubyte_color_iformat(usize num_channels, Colorspace colorspace)
    -> InternalFormat
{
    switch (colorspace)
    {
        case Colorspace::Linear:
            switch (num_channels)
            {
                case 3: return InternalFormat::RGB8;
                case 4: return InternalFormat::RGBA8;
                default: break;
            }
            break;
        case Colorspace::sRGB:
            switch (num_channels)
            {
                case 3: return InternalFormat::SRGB8;
                case 4: return InternalFormat::SRGBA8;
                default: break;
            }
            break;
    }
    panic_fmt("Invalid number of image channels: {}.", num_channels);
}

/*
Red/RG/RGB/RGBA from num_channels.
PRE: num_channels: [1, 4].
*/
inline auto base_pdformat_from_num_channels(usize num_channels)
    -> PixelDataFormat
{
    switch (num_channels)
    {
        case 1: return PixelDataFormat::Red;
        case 2: return PixelDataFormat::RG;
        case 3: return PixelDataFormat::RGB;
        case 4: return PixelDataFormat::RGBA;
        default: panic_fmt("Invalid number of image channels: {}.", num_channels);
    }
}

/*
Given an existing swizzle and a next one applied on top, returns
a new swizzle that corresponds to two consecutive swizzles applied
as `next(existing(source))`.

NOTE: This is not the same as just setting `next` as the new swizzle
of a given texture - that would be equivalent to going from `exising(source)`
to `next(source)`.

TODO: This should be in more common vocabulary.
*/
inline auto fold_swizzle(const SwizzleRGBA& existing, const SwizzleRGBA& next)
    -> SwizzleRGBA
{
    using enum Swizzle;

    auto get_slot = [](const SwizzleRGBA& src, Swizzle slot)
        -> Swizzle
    {
        switch (slot)
        {
            case Red:   return src.r;
            case Green: return src.g;
            case Blue:  return src.b;
            case Alpha: return src.a;
            case One:   return One;
            case Zero:  return Zero;
        };
    };

    return {
        .r = get_slot(existing, get_slot(next, Red)),
        .g = get_slot(existing, get_slot(next, Green)),
        .b = get_slot(existing, get_slot(next, Blue)),
        .a = get_slot(existing, get_slot(next, Alpha)),
    };
}


} // namespace josh
