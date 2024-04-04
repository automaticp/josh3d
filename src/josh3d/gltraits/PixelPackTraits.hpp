#pragma once
#include "GLPixelPackTraits.hpp"
#include "Pixels.hpp"


namespace josh {


#define JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(PixelT, Format, Type)          \
    template<> struct pixel_pack_traits<PixelT> {                          \
        static constexpr PixelDataFormat format = PixelDataFormat::Format; \
        static constexpr PixelDataType   type   = PixelDataType::Type;     \
    };


JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::RED,   Red,  UByte)
JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::RG,    RG,   UByte)
JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::RGB,   RGB,  UByte)
JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::RGBA,  RGBA, UByte)

JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::REDF,  Red,  Float)
JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::RGF,   RG,   Float)
JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::RGBF,  RGB,  Float)
JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS(pixel::RGBAF, RGBA, Float)


#undef JOSH3D_SPECIALIZE_PIXEL_PACK_TRAITS


} // namespace josh
