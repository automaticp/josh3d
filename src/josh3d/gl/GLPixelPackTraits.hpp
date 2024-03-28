#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAPI.hpp"
#include "GLScalars.hpp"



namespace josh {


enum class PixelDataFormat : GLuint {
    StencilIndex   = GLuint(gl::GL_STENCIL_INDEX),
    DepthComponent = GLuint(gl::GL_DEPTH_COMPONENT),
    DepthStencil   = GLuint(gl::GL_DEPTH_STENCIL),

    Red            = GLuint(gl::GL_RED),
    Green          = GLuint(gl::GL_GREEN),
    Blue           = GLuint(gl::GL_BLUE),

    RG             = GLuint(gl::GL_RG),
    RGB            = GLuint(gl::GL_RGB),
    RGBA           = GLuint(gl::GL_RGBA),

    BGR            = GLuint(gl::GL_BGR),
    BGRA           = GLuint(gl::GL_BGRA),

    RedInteger     = GLuint(gl::GL_RED_INTEGER),
    GreenInteger   = GLuint(gl::GL_GREEN_INTEGER),
    BlueInteger    = GLuint(gl::GL_BLUE_INTEGER),

    RGInteger      = GLuint(gl::GL_RG_INTEGER),
    RGBInteger     = GLuint(gl::GL_RGB_INTEGER),
    RGBAInteger    = GLuint(gl::GL_RGBA_INTEGER),

    BGRInteger     = GLuint(gl::GL_BGR_INTEGER),
    BGRAInteger    = GLuint(gl::GL_BGRA_INTEGER),
};


enum class PixelDataType : GLuint {
    UByte                  = GLuint(gl::GL_UNSIGNED_BYTE),
    Byte                   = GLuint(gl::GL_BYTE),
    UShort                 = GLuint(gl::GL_UNSIGNED_SHORT),
    Short                  = GLuint(gl::GL_SHORT),
    UInt                   = GLuint(gl::GL_UNSIGNED_INT),
    Int                    = GLuint(gl::GL_INT),

    HalfFloat              = GLuint(gl::GL_HALF_FLOAT),
    Float                  = GLuint(gl::GL_FLOAT),

    UByte_3_3_2            = GLuint(gl::GL_UNSIGNED_BYTE_3_3_2),
    UByte_2_3_3_Rev        = GLuint(gl::GL_UNSIGNED_BYTE_2_3_3_REV),

    UShort_5_6_5           = GLuint(gl::GL_UNSIGNED_SHORT_5_6_5),
    UShort_5_6_5_Rev       = GLuint(gl::GL_UNSIGNED_SHORT_5_6_5_REV),
    UShort_4_4_4_4         = GLuint(gl::GL_UNSIGNED_SHORT_4_4_4_4),
    UShort_4_4_4_4_Rev     = GLuint(gl::GL_UNSIGNED_SHORT_4_4_4_4_REV),
    UShort_5_5_5_1         = GLuint(gl::GL_UNSIGNED_SHORT_5_5_5_1),
    UShort_1_5_5_5_Rev     = GLuint(gl::GL_UNSIGNED_SHORT_1_5_5_5_REV),

    UInt_8_8_8_8           = GLuint(gl::GL_UNSIGNED_INT_8_8_8_8),
    UInt_8_8_8_8_Rev       = GLuint(gl::GL_UNSIGNED_INT_8_8_8_8_REV),
    UInt_10_10_10_2        = GLuint(gl::GL_UNSIGNED_INT_10_10_10_2),
    UInt_2_10_10_10_Rev    = GLuint(gl::GL_UNSIGNED_INT_2_10_10_10_REV),
    UInt_10F_11F_11F_Rev   = GLuint(gl::GL_UNSIGNED_INT_10F_11F_11F_REV),
    UInt_5_9_9_9_Rev       = GLuint(gl::GL_UNSIGNED_INT_5_9_9_9_REV),

    UInt_24_8              = GLuint(gl::GL_UNSIGNED_INT_24_8),

    Float_32_UInt_24_8_Rev = GLuint(gl::GL_FLOAT_32_UNSIGNED_INT_24_8_REV),
};






template<typename PixelT>
struct pixel_pack_traits;


template<typename PixelT>
concept specifies_pixel_pack_traits = requires {
    { pixel_pack_traits<PixelT>::format } -> same_as_remove_cvref<PixelDataFormat>;
    { pixel_pack_traits<PixelT>::type   } -> same_as_remove_cvref<PixelDataType>;
};


/*
// EXAMPLE SPECIALIZATION:

struct MyPixel {
    float r, g, b, a;
};

template<> struct pixel_pack_traits<MyPixel> {
    static constexpr PixelDataFormat format = PixelDataFormat::RGBA;
    static constexpr PixelDataType   type   = PixelDataType::Float;
};

struct MyPixelInt {
    unsigned int r, g;
};

template<> struct pixel_pack_traits<MyPixelInt> {
    static constexpr PixelDataFormat format = PixelDataFormat::RGInteger;
    static constexpr PixelDataType   type   = PixelDataType::UInt;
};

*/


} // namespace josh
