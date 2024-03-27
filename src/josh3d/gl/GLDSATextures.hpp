#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAPI.hpp"
#include "GLDSABuffers.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "Index.hpp"
#include "PixelPackTraits.hpp"
#include "Region.hpp"
#include "Size.hpp"
#include "detail/ConditionalMixin.hpp"
#include "detail/EnumCompatability.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include "detail/StaticAssertFalseMacro.hpp"
#include "detail/StrongScalar.hpp"
#include <cstddef>
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/types.h>
#include <type_traits>
#include <span>
#include <utility>


namespace josh {




/*
Since each texture target is a distinct type on-construction
for DSA style, we use the texutre target as a reflection enum.

Names are Uppercase to match Type capitalization. Helps in macros.
*/
enum class TextureTarget : GLuint {
    Texture1D        = GLuint(gl::GL_TEXTURE_1D),                   // No precedent.
    Texture1DArray   = GLuint(gl::GL_TEXTURE_1D_ARRAY),             // No precedent.
    Texture2D        = GLuint(gl::GL_TEXTURE_2D),
    Texture2DArray   = GLuint(gl::GL_TEXTURE_2D_ARRAY),
    Texture2DMS      = GLuint(gl::GL_TEXTURE_2D_MULTISAMPLE),
    Texture2DMSArray = GLuint(gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY), // No precedent.
    Texture3D        = GLuint(gl::GL_TEXTURE_3D),                   // No precedent.
    Cubemap          = GLuint(gl::GL_TEXTURE_CUBE_MAP),
    CubemapArray     = GLuint(gl::GL_TEXTURE_CUBE_MAP_ARRAY),
    TextureRectangle = GLuint(gl::GL_TEXTURE_RECTANGLE),            // No precedent.
    TextureBuffer    = GLuint(gl::GL_TEXTURE_BUFFER),               // No precedent.
};







enum class Swizzle : GLuint {
    Red   = GLuint(gl::GL_RED),
    Green = GLuint(gl::GL_GREEN),
    Blue  = GLuint(gl::GL_BLUE),
    Alpha = GLuint(gl::GL_ALPHA),
    Zero  = GLuint(gl::GL_ZERO),
    One   = GLuint(gl::GL_ONE),
};


struct SwizzleRGBA {
    Swizzle r{ Swizzle::Red   };
    Swizzle g{ Swizzle::Green };
    Swizzle b{ Swizzle::Blue  };
    Swizzle a{ Swizzle::Alpha };
};


enum class DepthStencilTarget : GLuint {
    DepthComponent = GLuint(gl::GL_DEPTH_COMPONENT),
    StencilIndex   = GLuint(gl::GL_STENCIL_INDEX),
};







/*
Strong integer type meant to be used at the call-site to disambiguate
integer paramters of certail funtions.

Compare weak integers:
    `fbo.attach_texture_layer_to_color_buffer(tex, 3, 1, 0);`

To using strong integer types:
    `fbo.attach_texture_layer_to_color_buffer(tex, Layer{ 3 }, 1, MipLevel{ 0 });`

*/
JOSH3D_DEFINE_STRONG_SCALAR(Layer, GLint)


/*
Strong integer type meant to be used at the call-site to disambiguate
integer paramters of certail funtions.

Compare weak integers:
    `fbo.attach_texture_layer_to_color_buffer(tex, 3, 1, 0);`

To using strong integer types:
    `fbo.attach_texture_layer_to_color_buffer(tex, Layer{ 3 }, 1, MipLevel{ 0 });`

*/
JOSH3D_DEFINE_STRONG_SCALAR(MipLevel, GLint)
JOSH3D_DEFINE_STRONG_SCALAR(NumLevels, GLsizei)




/*
"[8.8] samples represents a request for a desired minimum number of samples.
Since different implementations may support different sample counts for multi-
sampled textures, the actual number of samples allocated for the texture image is
implementation-dependent. However, the resulting value for TEXTURE_SAMPLES
is guaranteed to be greater than or equal to samples and no more than the next
larger sample count supported by the implementation."
*/
JOSH3D_DEFINE_STRONG_SCALAR(NumSamples, GLsizei)


enum struct SampleLocations : bool {
    NotFixed = false,
    Fixed    = true,
};



/*
"[???] internalformat may be specified as one of the internal format
symbolic constants listed in table 8.11, as one of the sized internal format symbolic
constants listed in tables 8.12-8.13, as one of the generic compressed internal
format symbolic constants listed in table 8.14, or as one of the specific compressed
internal format symbolic constants (if listed in table 8.14)."
*/
enum class InternalFormat : GLuint {
    // Base Internal Formats.
    Red            = GLuint(gl::GL_RED),
    RG             = GLuint(gl::GL_RG),
    RGB            = GLuint(gl::GL_RGB),
    RGBA           = GLuint(gl::GL_RGBA),
    DepthComponent = GLuint(gl::GL_DEPTH_COMPONENT),
    DepthStencil   = GLuint(gl::GL_DEPTH_STENCIL),
    StencilIndex   = GLuint(gl::GL_STENCIL_INDEX),
    // Sized Internal Formats.
    R8                = GLuint(gl::GL_R8),
    R8_SNorm          = GLuint(gl::GL_R8_SNORM),
    R16               = GLuint(gl::GL_R16),
    R16_SNorm         = GLuint(gl::GL_R16_SNORM),
    RG8               = GLuint(gl::GL_RG8),
    RG8_SNorm         = GLuint(gl::GL_RG8_SNORM),
    RG16              = GLuint(gl::GL_RG16),
    RG16_SNorm        = GLuint(gl::GL_RG16_SNORM),
    R3_G3_B2          = GLuint(gl::GL_R3_G3_B2),
    RGB4              = GLuint(gl::GL_RGB4),
    RGB5              = GLuint(gl::GL_RGB5),
    R5_G6_B5          = GLuint(gl::GL_RGB565),
    RGB8              = GLuint(gl::GL_RGB8),
    RGB8_SNorm        = GLuint(gl::GL_RGB8_SNORM),
    RGB10             = GLuint(gl::GL_RGB10),
    RGB12             = GLuint(gl::GL_RGB12),
    RGB16             = GLuint(gl::GL_RGB16),
    RGB16_SNorm       = GLuint(gl::GL_RGB16_SNORM),
    RGBA2             = GLuint(gl::GL_RGBA2),
    RGBA4             = GLuint(gl::GL_RGBA4),
    RGB5_A1           = GLuint(gl::GL_RGB5_A1),
    RGBA8             = GLuint(gl::GL_RGBA8),
    RGBA8_SNorm       = GLuint(gl::GL_RGBA8_SNORM),
    RGB10_A2          = GLuint(gl::GL_RGB10_A2),
    RGB10_A2UI        = GLuint(gl::GL_RGB10_A2UI),
    RGBA12            = GLuint(gl::GL_RGBA12),
    RGBA16            = GLuint(gl::GL_RGBA16),
    RGBA16_SNorm      = GLuint(gl::GL_RGBA16_SNORM),
    SRGB8             = GLuint(gl::GL_SRGB8),
    SRGBA8            = GLuint(gl::GL_SRGB8_ALPHA8),
    R16F              = GLuint(gl::GL_R16F),
    RG16F             = GLuint(gl::GL_RG16F),
    RGB16F            = GLuint(gl::GL_RGB16F),
    RGBA16F           = GLuint(gl::GL_RGBA16F),
    R32F              = GLuint(gl::GL_R32F),
    RG32F             = GLuint(gl::GL_RG32F),
    RGB32F            = GLuint(gl::GL_RGB32F),
    RGBA32F           = GLuint(gl::GL_RGBA32F),
    R11F_G11F_B10F    = GLuint(gl::GL_R11F_G11F_B10F),
    RGB9_E5           = GLuint(gl::GL_RGB9_E5),
    R8I               = GLuint(gl::GL_R8I),
    R8UI              = GLuint(gl::GL_R8UI),
    R16I              = GLuint(gl::GL_R16I),
    R16UI             = GLuint(gl::GL_R16UI),
    R32I              = GLuint(gl::GL_R32I),
    R32UI             = GLuint(gl::GL_R32UI),
    RG8I              = GLuint(gl::GL_RG8I),
    RG8UI             = GLuint(gl::GL_RG8UI),
    RG16I             = GLuint(gl::GL_RG16I),
    RG16UI            = GLuint(gl::GL_RG16UI),
    RG32I             = GLuint(gl::GL_RG32I),
    RG32UI            = GLuint(gl::GL_RG32UI),
    RGB8I             = GLuint(gl::GL_RGB8I),
    RGB8UI            = GLuint(gl::GL_RGB8UI),
    RGB16I            = GLuint(gl::GL_RGB16I),
    RGB16UI           = GLuint(gl::GL_RGB16UI),
    RGB32I            = GLuint(gl::GL_RGB32I),
    RGB32UI           = GLuint(gl::GL_RGB32UI),
    RGBA8I            = GLuint(gl::GL_RGBA8I),
    RGBA8UI           = GLuint(gl::GL_RGBA8UI),
    RGBA16I           = GLuint(gl::GL_RGBA16I),
    RGBA16UI          = GLuint(gl::GL_RGBA16UI),
    RGBA32I           = GLuint(gl::GL_RGBA32I),
    RGBA32UI          = GLuint(gl::GL_RGBA32UI),
    DepthComponent16  = GLuint(gl::GL_DEPTH_COMPONENT16),
    DepthComponent24  = GLuint(gl::GL_DEPTH_COMPONENT24),
    DepthComponent32  = GLuint(gl::GL_DEPTH_COMPONENT32),
    DepthComponent32F = GLuint(gl::GL_DEPTH_COMPONENT32F),
    Depth24_Stencil8  = GLuint(gl::GL_DEPTH24_STENCIL8),
    Depth32F_Stencil8 = GLuint(gl::GL_DEPTH32F_STENCIL8),
    StencilIndex1     = GLuint(gl::GL_STENCIL_INDEX1),
    StencilIndex4     = GLuint(gl::GL_STENCIL_INDEX4),
    StencilIndex8     = GLuint(gl::GL_STENCIL_INDEX8),
    StencilIndex16    = GLuint(gl::GL_STENCIL_INDEX16),
    // Generic Compressed Internal Formats.
    Compressed_Red   = GLuint(gl::GL_COMPRESSED_RED),
    Compressed_RG    = GLuint(gl::GL_COMPRESSED_RG),
    Compressed_RGB   = GLuint(gl::GL_COMPRESSED_RGB),
    Compressed_RGBA  = GLuint(gl::GL_COMPRESSED_RGBA),
    Compressed_SRGB  = GLuint(gl::GL_COMPRESSED_SRGB),
    Compressed_SRGBA = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA),
    // Specific Compressed Internal Formats.
    Compressed_Red_RGTC1                  = GLuint(gl::GL_COMPRESSED_RED_RGTC1),
    Compressed_Red_RGTC1_SNorm            = GLuint(gl::GL_COMPRESSED_SIGNED_RED_RGTC1),
    Compressed_RG_RGTC2                   = GLuint(gl::GL_COMPRESSED_RG_RGTC2),
    Compressed_RG_RGTC2_SNorm             = GLuint(gl::GL_COMPRESSED_SIGNED_RG_RGTC2),
    Compressed_RGBA_BPTC_UNorm            = GLuint(gl::GL_COMPRESSED_RGBA_BPTC_UNORM),
    Compressed_SRGBA_BPTC_UNorm           = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM),
    Compressed_RGB_BPTC_SignedFloat       = GLuint(gl::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT),
    Compressed_RGB_BPTC_UnsignedFloat     = GLuint(gl::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT),
    Compressed_RGB8_ETC2                  = GLuint(gl::GL_COMPRESSED_RGB8_ETC2),
    Compressed_SRGB8_ETC2                 = GLuint(gl::GL_COMPRESSED_SRGB8_ETC2),
    Compressed_RGB8_Punchthrough_A1_ETC2  = GLuint(gl::GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2),
    Compressed_SRGB8_Punchthrough_A1_ETC2 = GLuint(gl::GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2),
    Compressed_RGBA8_ETC2_EAC             = GLuint(gl::GL_COMPRESSED_RGBA8_ETC2_EAC),
    Compressed_SRGBA8_ETC2_EAC            = GLuint(gl::GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC),
    Compressed_R11_EAC                    = GLuint(gl::GL_COMPRESSED_R11_EAC),
    Compressed_R11_EAC_SNorm              = GLuint(gl::GL_COMPRESSED_SIGNED_R11_EAC),
    Compressed_RG11_EAC                   = GLuint(gl::GL_COMPRESSED_RG11_EAC),
    Compressed_RG11_EAC_SNorm             = GLuint(gl::GL_COMPRESSED_SIGNED_RG11_EAC),
    // GL_EXT_texture_compression_s3tc
    Compressed_RGB_S3TC_DXT1_EXT          = GLuint(gl::GL_COMPRESSED_RGB_S3TC_DXT1_EXT),
    Compressed_RGBA_S3TC_DXT1_EXT         = GLuint(gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT),
    Compressed_RGBA_S3TC_DXT3_EXT         = GLuint(gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT),
    Compressed_RGBA_S3TC_DXT5_EXT         = GLuint(gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT),
    // GL_EXT_texture_sRGB
    Compressed_SRGB_S3TC_DXT1_EXT         = GLuint(gl::GL_COMPRESSED_SRGB_S3TC_DXT1_EXT),
    Compressed_SRGBA_S3TC_DXT1_EXT        = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT),
    Compressed_SRGBA_S3TC_DXT3_EXT        = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT),
    Compressed_SRGBA_S3TC_DXT5_EXT        = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT),
};


enum class CompressedInternalFormat : GLuint {
    Compressed_Red_RGTC1                  = GLuint(gl::GL_COMPRESSED_RED_RGTC1),
    Compressed_Red_RGTC1_SNorm            = GLuint(gl::GL_COMPRESSED_SIGNED_RED_RGTC1),
    Compressed_RG_RGTC2                   = GLuint(gl::GL_COMPRESSED_RG_RGTC2),
    Compressed_RG_RGTC2_SNorm             = GLuint(gl::GL_COMPRESSED_SIGNED_RG_RGTC2),
    Compressed_RGBA_BPTC_UNorm            = GLuint(gl::GL_COMPRESSED_RGBA_BPTC_UNORM),
    Compressed_SRGBA_BPTC_UNorm           = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM),
    Compressed_RGB_BPTC_SignedFloat       = GLuint(gl::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT),
    Compressed_RGB_BPTC_UnsignedFloat     = GLuint(gl::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT),
    Compressed_RGB8_ETC2                  = GLuint(gl::GL_COMPRESSED_RGB8_ETC2),
    Compressed_SRGB8_ETC2                 = GLuint(gl::GL_COMPRESSED_SRGB8_ETC2),
    Compressed_RGB8_Punchthrough_A1_ETC2  = GLuint(gl::GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2),
    Compressed_SRGB8_Punchthrough_A1_ETC2 = GLuint(gl::GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2),
    Compressed_RGBA8_ETC2_EAC             = GLuint(gl::GL_COMPRESSED_RGBA8_ETC2_EAC),
    Compressed_SRGBA8_ETC2_EAC            = GLuint(gl::GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC),
    Compressed_R11_EAC                    = GLuint(gl::GL_COMPRESSED_R11_EAC),
    Compressed_R11_EAC_SNorm              = GLuint(gl::GL_COMPRESSED_SIGNED_R11_EAC),
    Compressed_RG11_EAC                   = GLuint(gl::GL_COMPRESSED_RG11_EAC),
    Compressed_RG11_EAC_SNorm             = GLuint(gl::GL_COMPRESSED_SIGNED_RG11_EAC),
    // GL_EXT_texture_compression_s3tc
    Compressed_RGB_S3TC_DXT1_EXT          = GLuint(gl::GL_COMPRESSED_RGB_S3TC_DXT1_EXT),
    Compressed_RGBA_S3TC_DXT1_EXT         = GLuint(gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT),
    Compressed_RGBA_S3TC_DXT3_EXT         = GLuint(gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT),
    Compressed_RGBA_S3TC_DXT5_EXT         = GLuint(gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT),
    // GL_EXT_texture_sRGB
    Compressed_SRGB_S3TC_DXT1_EXT         = GLuint(gl::GL_COMPRESSED_SRGB_S3TC_DXT1_EXT),
    Compressed_SRGBA_S3TC_DXT1_EXT        = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT),
    Compressed_SRGBA_S3TC_DXT3_EXT        = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT),
    Compressed_SRGBA_S3TC_DXT5_EXT        = GLuint(gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT),
};

JOSH3D_DECLARE_ENUM_AS_SUPERSET(InternalFormat, CompressedInternalFormat)


enum class ImageUnitFormat : GLuint {
    RGBA32F        = GLuint(gl::GL_RGBA32F),        // rgba32f
    RGBA16F        = GLuint(gl::GL_RGBA16F),        // rgba16f
    RG32F          = GLuint(gl::GL_RG32F),          // rg32f
    RG16F          = GLuint(gl::GL_RG16F),          // rg16f
    R11F_G11F_B10F = GLuint(gl::GL_R11F_G11F_B10F), // r11f_g11f_b10f
    R32F           = GLuint(gl::GL_R32F),           // r32f
    R16F           = GLuint(gl::GL_R16F),           // r16f
    RGBA32UI       = GLuint(gl::GL_RGBA32UI),       // rgba32ui
    RGBA16UI       = GLuint(gl::GL_RGBA16UI),       // rgba16ui
    RGB10_A2UI     = GLuint(gl::GL_RGB10_A2UI),     // rgb10_a2ui
    RGBA8UI        = GLuint(gl::GL_RGBA8UI),        // rgba8ui
    RG32UI         = GLuint(gl::GL_RG32UI),         // rg32ui
    RG16UI         = GLuint(gl::GL_RG16UI),         // rg16ui
    RG8UI          = GLuint(gl::GL_RG8UI),          // rg8ui
    R32UI          = GLuint(gl::GL_R32UI),          // r32ui
    R16UI          = GLuint(gl::GL_R16UI),          // r16ui
    R8UI           = GLuint(gl::GL_R8UI),           // r8ui
    RGBA32I        = GLuint(gl::GL_RGBA32I),        // rgba32i
    RGBA16I        = GLuint(gl::GL_RGBA16I),        // rgba16i
    RGBA8I         = GLuint(gl::GL_RGBA8I),         // rgba8i
    RG32I          = GLuint(gl::GL_RG32I),          // rg32i
    RG16I          = GLuint(gl::GL_RG16I),          // rg16i
    RG8I           = GLuint(gl::GL_RG8I),           // rg8i
    R32I           = GLuint(gl::GL_R32I),           // r32i
    R16I           = GLuint(gl::GL_R16I),           // r16i
    R8I            = GLuint(gl::GL_R8I),            // r8i
    RGBA16         = GLuint(gl::GL_RGBA16),         // rgba16
    RGB10_A2       = GLuint(gl::GL_RGB10_A2),       // rgb10_a2
    RGBA8          = GLuint(gl::GL_RGBA8),          // rgba8
    RG16           = GLuint(gl::GL_RG16),           // rg16
    RG8            = GLuint(gl::GL_RG8),            // rg8
    R16            = GLuint(gl::GL_R16),            // r16
    R8             = GLuint(gl::GL_R8),             // r8
    RGBA16_SNorm   = GLuint(gl::GL_RGBA16_SNORM),   // rgba16_snorm
    RGBA8_SNorm    = GLuint(gl::GL_RGBA8_SNORM),    // rgba8_snorm
    RG16_SNorm     = GLuint(gl::GL_RG16_SNORM),     // rg16_snorm
    RG8_SNorm      = GLuint(gl::GL_RG8_SNORM),      // rg8_snorm
    R16_SNorm      = GLuint(gl::GL_R16_SNORM),      // r16_snorm
    R8_SNorm       = GLuint(gl::GL_R8_SNORM),       // r8_snorm
};

JOSH3D_DECLARE_ENUM_AS_SUPERSET(InternalFormat, ImageUnitFormat)


enum class ImageUnitFormatCompatibility : GLuint {
    None    = GLuint(gl::GL_NONE),
    BySize  = GLuint(gl::GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE),
    ByClass = GLuint(gl::GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS),
};


enum class BufferTextureInternalFormat : GLuint {
    R8       = GLuint(gl::GL_R8),
    R16      = GLuint(gl::GL_R16),
    R16F     = GLuint(gl::GL_R16F),
    R32F     = GLuint(gl::GL_R32F),
    R8I      = GLuint(gl::GL_R8I),
    R16I     = GLuint(gl::GL_R16I),
    R32I     = GLuint(gl::GL_R32I),
    R8UI     = GLuint(gl::GL_R8UI),
    R16UI    = GLuint(gl::GL_R16UI),
    R32UI    = GLuint(gl::GL_R32UI),
    RG8      = GLuint(gl::GL_RG8),
    RG16     = GLuint(gl::GL_RG16),
    RG16F    = GLuint(gl::GL_RG16F),
    RG32F    = GLuint(gl::GL_RG32F),
    RG8I     = GLuint(gl::GL_RG8I),
    RG16I    = GLuint(gl::GL_RG16I),
    RG32I    = GLuint(gl::GL_RG32I),
    RG8UI    = GLuint(gl::GL_RG8UI),
    RG16UI   = GLuint(gl::GL_RG16UI),
    RG32UI   = GLuint(gl::GL_RG32UI),
    RGB32F   = GLuint(gl::GL_RGB32F),
    RGB32I   = GLuint(gl::GL_RGB32I),
    RGB32UI  = GLuint(gl::GL_RGB32UI),
    RGBA8    = GLuint(gl::GL_RGBA8),
    RGBA16   = GLuint(gl::GL_RGBA16),
    RGBA16F  = GLuint(gl::GL_RGBA16F),
    RGBA32F  = GLuint(gl::GL_RGBA32F),
    RGBA8I   = GLuint(gl::GL_RGBA8I),
    RGBA16I  = GLuint(gl::GL_RGBA16I),
    RGBA32I  = GLuint(gl::GL_RGBA32I),
    RGBA8UI  = GLuint(gl::GL_RGBA8UI),
    RGBA16UI = GLuint(gl::GL_RGBA16UI),
    RGBA32UI = GLuint(gl::GL_RGBA32UI),
};

JOSH3D_DECLARE_ENUM_AS_SUPERSET(InternalFormat, BufferTextureInternalFormat)


enum class PixelComponent {
    Red,
    Green,
    Blue,
    Alpha,
    Depth,
    Stencil,
    SharedExponent,
};


enum class PixelComponentType : GLuint {
    None            = GLuint(gl::GL_NONE),
    SNorm           = GLuint(gl::GL_SIGNED_NORMALIZED),
    UNorm           = GLuint(gl::GL_UNSIGNED_NORMALIZED),
    Float           = GLuint(gl::GL_FLOAT),
    Integer         = GLuint(gl::GL_INT),
    UnsignedInteger = GLuint(gl::GL_UNSIGNED_INT)
};





namespace dsa {


namespace detail {

template<TextureTarget> struct is_multisample                     : std::false_type {};
template<> struct is_multisample<TextureTarget::Texture2DMS>      : std::true_type  {};
template<> struct is_multisample<TextureTarget::Texture2DMSArray> : std::true_type  {};

template<TextureTarget> struct has_lod                     : std::true_type  {};
template<> struct has_lod<TextureTarget::Texture2DMS>      : std::false_type {};
template<> struct has_lod<TextureTarget::Texture2DMSArray> : std::false_type {};
template<> struct has_lod<TextureTarget::TextureRectangle> : std::false_type {};
template<> struct has_lod<TextureTarget::TextureBuffer>    : std::false_type {};

/*
"[9.2.8] If texture is the name of a three-dimensional texture, cube map array texture,
cube map texture, one- or two-dimensional array texture, or two-dimensional multisample
array texture, the texture level attached to the framebuffer attachment point
is an array of images, and the framebuffer attachment is considered layered."
*/
template<TextureTarget> struct is_layered                     : std::false_type {};
template<> struct is_layered<TextureTarget::Texture1DArray>   : std::true_type  {};
template<> struct is_layered<TextureTarget::Texture2DArray>   : std::true_type  {};
template<> struct is_layered<TextureTarget::Texture2DMSArray> : std::true_type  {};
template<> struct is_layered<TextureTarget::Texture3D>        : std::true_type  {};
template<> struct is_layered<TextureTarget::Cubemap>          : std::true_type  {};
template<> struct is_layered<TextureTarget::CubemapArray>     : std::true_type  {};

template<TextureTarget> struct is_array_texture                     : std::false_type {};
template<> struct is_array_texture<TextureTarget::Texture1DArray>   : std::true_type  {};
template<> struct is_array_texture<TextureTarget::Texture2DArray>   : std::true_type  {};
template<> struct is_array_texture<TextureTarget::Texture2DMSArray> : std::true_type  {};
template<> struct is_array_texture<TextureTarget::CubemapArray>     : std::true_type  {};

template<TextureTarget> struct supports_compressed_internal_format                     : std::true_type  {};
template<> struct supports_compressed_internal_format<TextureTarget::TextureBuffer>    : std::false_type {};
template<> struct supports_compressed_internal_format<TextureTarget::TextureRectangle> : std::false_type {};
template<> struct supports_compressed_internal_format<TextureTarget::Texture2DMS>      : std::false_type {};
template<> struct supports_compressed_internal_format<TextureTarget::Texture2DMSArray> : std::false_type {};


template<typename> struct size_dims;
template<specialization_of<Extent1> S> struct size_dims<S> : std::integral_constant<size_t, 1> {};
template<specialization_of<Extent2> S> struct size_dims<S> : std::integral_constant<size_t, 2> {};
template<specialization_of<Extent3> S> struct size_dims<S> : std::integral_constant<size_t, 3> {};


template<TextureTarget> struct texture_resolution;
template<> struct texture_resolution<TextureTarget::Texture1D>        { using type = Size1I; };
template<> struct texture_resolution<TextureTarget::Texture1DArray>   { using type = Size1I; };
template<> struct texture_resolution<TextureTarget::Texture2D>        { using type = Size2I; };
template<> struct texture_resolution<TextureTarget::Texture2DArray>   { using type = Size2I; };
template<> struct texture_resolution<TextureTarget::Texture2DMS>      { using type = Size2I; };
template<> struct texture_resolution<TextureTarget::Texture2DMSArray> { using type = Size2I; };
template<> struct texture_resolution<TextureTarget::Texture3D>        { using type = Size3I; };
template<> struct texture_resolution<TextureTarget::Cubemap>          { using type = Size2I; };
template<> struct texture_resolution<TextureTarget::CubemapArray>     { using type = Size2I; };
template<> struct texture_resolution<TextureTarget::TextureRectangle> { using type = Size2I; };
template<> struct texture_resolution<TextureTarget::TextureBuffer>    { using type = Size1I; };


template<TextureTarget TargetV> struct texture_resolution_dims : size_dims<typename texture_resolution<TargetV>::type> {};


template<TextureTarget> struct texture_region_dims;
template<> struct texture_region_dims<TextureTarget::Texture1D>        : std::integral_constant<size_t, 1> {};
template<> struct texture_region_dims<TextureTarget::Texture1DArray>   : std::integral_constant<size_t, 2> {};
template<> struct texture_region_dims<TextureTarget::Texture2D>        : std::integral_constant<size_t, 2> {};
template<> struct texture_region_dims<TextureTarget::Texture2DArray>   : std::integral_constant<size_t, 3> {};
template<> struct texture_region_dims<TextureTarget::Texture2DMS>      : std::integral_constant<size_t, 2> {};
template<> struct texture_region_dims<TextureTarget::Texture2DMSArray> : std::integral_constant<size_t, 3> {};
template<> struct texture_region_dims<TextureTarget::Texture3D>        : std::integral_constant<size_t, 3> {};
template<> struct texture_region_dims<TextureTarget::Cubemap>          : std::integral_constant<size_t, 3> {};
template<> struct texture_region_dims<TextureTarget::CubemapArray>     : std::integral_constant<size_t, 3> {};
template<> struct texture_region_dims<TextureTarget::TextureRectangle> : std::integral_constant<size_t, 2> {};
template<> struct texture_region_dims<TextureTarget::TextureBuffer>    : std::integral_constant<size_t, 1> {};


template<size_t NDimsV> struct texture_region_dims_traits;
template<> struct texture_region_dims_traits<1> { using offset_type = Offset1I; using extent_type = Extent1I; using region_type = Region1I; };
template<> struct texture_region_dims_traits<2> { using offset_type = Offset2I; using extent_type = Extent2I; using region_type = Region2I; };
template<> struct texture_region_dims_traits<3> { using offset_type = Offset3I; using extent_type = Extent3I; using region_type = Region3I; };


template<TextureTarget TargetV> struct texture_region_traits {
    using offset_type = texture_region_dims_traits<texture_region_dims<TargetV>::value>::offset_type;
    using extent_type = texture_region_dims_traits<texture_region_dims<TargetV>::value>::extent_type;
    using region_type = texture_region_dims_traits<texture_region_dims<TargetV>::value>::region_type;
    static constexpr GLsizeiptr ndims = texture_region_dims<TargetV>::value;
};










} // namespace detail




template<TextureTarget TargetV>
struct texture_target_traits {

    /*
    True dimensionality of the texture data, ignoring composition into arrays and cubemaps.

    Used in storage alloacation functions as the primary argument.
    */
    using resolution_type                        = detail::texture_resolution<TargetV>::type;
    static constexpr GLsizeiptr resolution_ndims = detail::texture_resolution_dims<TargetV>::value;


    /*
    Extent type defines the size used in operations that have to "index into" a Region
    of a texture such as `glTextureSubImage*`, `glCopyTextureSubImage*`, etc.

    Cubemaps are an exception to how their size is specified betweed `glTextureStorage*`
    and `glTextureSubImage*` commands. For allocation of storage we use `glTextureStorage2D`
    where the size is 2-dimensions and allocates the same 2d storage for all 6 faces, whereas
    for submitting data we have to use `glTextureSubImage3D` with 3d size where the `depth`
    represents the face number.

    - For cubemaps and cubemap arrays, the size refers to the number of contiguous faces,
    where each cubemap in the array occupies 6 contiguous indices.

    The product of all dimensions of this size gives you the *total number of pixels* in
    a given texture level or its subregion.
    */
    using extent_type = detail::texture_region_traits<TargetV>::extent_type;

    /*
    Offset type defines the index offset for operations that have to "index into" a Region
    of a texture storage and is related to Extent.

    Offset + Extent forms a Region that is used in functions that operate on such.

    - For cubemaps and cubemap arrays, the index refers to a particular face,
    where each cubemap in the array occupies 6 contiguous indices.
    */
    using offset_type = detail::texture_region_traits<TargetV>::offset_type;

    /*
    Region is a pair of Offset and Extent of the same dimensionality.
    */
    using region_type = detail::texture_region_traits<TargetV>::region_type;

    /*
    Number of dimensions needed to fully define a texture Region.
    Applies to both Offset, Extent and Region types.
    */
    static constexpr GLsizeiptr region_ndims = detail::texture_region_dims<TargetV>::value;


    /*
    Arrays.
    */
    static constexpr bool is_array          = detail::is_array_texture<TargetV>::value;

    /*
    Multisample textures are a bit special in their semantics.
    They cannot have mipmap levels or filtering, and need special allocation functions
    and a separate internal storage spec such as `TexSpecMS`.
    */
    static constexpr bool is_multisample    = detail::is_multisample<TargetV>::value;

    /*
    Mipmaps are not present and are not allowed for multisample textures as well as
    for `GL_TEXTURE_RECTANGLE` (old stuff) and `GL_TEXTURE_BUFFER`.
    We can remove the level-of-detail parameter from certain overloads based on this.
    */
    static constexpr bool has_lod           = detail::has_lod<TargetV>::value;

    /*
    Layered textures are the Array textures, Cubemaps and the Texture3D. They can be attached by-layer
    to framebuffers and image units, as well as rendered-to using "layered rendering" in GS.
    */
    static constexpr bool is_layered        = detail::is_layered<TargetV>::value;


    /*
    InternalFormats of these texture types can be one of the compressed formats.
    */
    static constexpr bool supports_compressed_internal_format
                                            = detail::supports_compressed_internal_format<TargetV>::value;

};




template<typename RawTextureH>
struct texture_traits : texture_target_traits<RawTextureH::target_type> {};












// Below we compose Mixins of a final texture interface based on the texture traits.
// We choose to do it this way because:
//  1. We can conditionally disable parts of the interface with conditional_mixin_t;
//  2. It is easier to implement them in isolation.
namespace detail {


// This is inserted in every mixin type at the top.
// Saves some redundant lines that accumulate over ~30 mixin types.
// Assumes template parameters of `typename CRTP` and `TextureTarget TargetV`.
#define JOSH3D_TEXTURE_MIXIN_HEADER                                                  \
private:                                                                             \
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); } \
    using mt = mutability_traits<CRTP>;                                              \
    using tt = texture_target_traits<TargetV>;                                       \
public:



// TODO: Remove later.
using josh::detail::conditional_mixin_t;





template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_ResolutionAndExtent {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:
    GLsizei get_size_1d_impl(GLint level) const noexcept {
        GLint width;
        gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_WIDTH,  &width);
        return width;
    }
    Size2I get_size_2d_impl(GLint level) const noexcept {
        GLint width, height;
        gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_WIDTH,  &width);
        gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_HEIGHT, &height);
        return { width, height };
    }
    Size3I get_size_3d_impl(GLint level) const noexcept {
        GLint width, height, depth;
        gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_WIDTH,  &width);
        gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_HEIGHT, &height);
        gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_DEPTH,  &depth);
        return { width, height, depth };
    }
private:
    auto get_resolution_impl(MipLevel level) const noexcept
        -> tt::resolution_type
    {
        if constexpr (tt::resolution_ndims == 1) {
            return get_size_1d_impl(level);
        } else if constexpr (tt::resolution_ndims == 2) {
            return get_size_2d_impl(level);
        } else if constexpr (tt::resolution_ndims == 3) {
            return get_size_3d_impl(level);
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }
public:

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_[WIDTH|HEIGHT|DEPTH]`.
    auto get_resolution(MipLevel level = MipLevel{ 0 }) const noexcept
        -> tt::resolution_type
            requires tt::has_lod
    {
        return get_resolution_impl(level);
    }

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_[WIDTH|HEIGHT|DEPTH]` and `level = 0`.
    auto get_resolution() const noexcept
        -> tt::resolution_type
            requires (!tt::has_lod)
    {
        return get_resolution_impl(MipLevel{ 0 });
    }




private:
    auto get_extent_impl(MipLevel level) const noexcept
        -> tt::extent_type
    {
        if constexpr        (tt::region_ndims == 1) {
            return get_size_1d_impl(level);
        } else if constexpr (tt::region_ndims == 2) {
            return get_size_2d_impl(level);
        } else if constexpr (tt::region_ndims == 3) {
            if constexpr (TargetV == TextureTarget::Cubemap) {
                return { get_size_2d_impl(level), 6 };
            } else {
                return get_size_3d_impl(level);
            }
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }
public:

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_[WIDTH|HEIGHT|DEPTH]`.
    //
    // - For `Texture[1|2|3]D`: `width`, (`height` (and `depth`)) are
    // the resolution of the image level.
    //
    // - For `Texture1DArray`: `width` is the resolution of each layer, `height` is the number of array layers.
    //
    // - For `Texture2DArray`: `width` and `height` are the resolution of each layer, `depth` is the number of array layers.
    //
    // - For `Cubemap`: `width` and `height` are the resolution of each face, `depth` is always 6.
    //
    // - For `CubemapArray`: `width` and `height` are the resolution of each face, `depth` is
    // the total number of faces in the array and is always a multiple of 6.
    auto get_extent(MipLevel level = MipLevel{ 0 }) const noexcept
        -> tt::extent_type
            requires tt::has_lod
    {
        return get_extent_impl(level);
    }

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_[WIDTH|HEIGHT|DEPTH]` and `level = 0`.
    //
    // - For `TextureBuffer`, `Texture2DMS`, `TextureRectange`: `width` (and `height`) are the resolution of the image.
    //
    // - For `Texture2DMSArray`: `width` and `height` are the resolution of each layer, `depth` is the number of array layers.
    auto get_extent() const noexcept
        -> tt::extent_type
            requires (!tt::has_lod)
    {
        return get_extent_impl(MipLevel{ 0 });
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_ViewLike_NumLayers {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_VIEW_NUM_LAYERS`.
    auto get_num_layers() const noexcept
        -> GLsizei
            requires tt::is_layered
    {
        GLsizei num_layers;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_VIEW_NUM_LAYERS, &num_layers);
        return num_layers;
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_ViewLike_MinLayer {
    JOSH3D_TEXTURE_MIXIN_HEADER

    auto get_min_view_layer() const noexcept
        -> GLsizei
    {
        GLsizei min_layer;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_VIEW_MIN_LAYER, &min_layer);
        return min_layer;
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_ViewLike_Levels {
    JOSH3D_TEXTURE_MIXIN_HEADER

    auto get_num_storage_levels() const noexcept
        -> NumLevels
    {
        GLsizei num_levels;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_IMMUTABLE_LEVELS, &num_levels);
        return NumLevels{ num_levels };
    }

    auto get_min_view_level() const noexcept
        -> MipLevel
    {
        GLint level;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_VIEW_MIN_LEVEL, &level);
        return MipLevel{ level };
    }

    auto get_num_view_levels() const noexcept
        -> NumLevels
    {
        GLsizei levels;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_VIEW_NUM_LEVELS, &levels);
        return NumLevels{ levels };
    }

};





template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_ViewLike
    : conditional_mixin_t<
        texture_target_traits<TargetV>::is_layered,
        TextureDSAInterface_Queries_ViewLike_NumLayers<CRTP, TargetV>
    >
    // Makes sense to support this for any texture type that can view a layered texture.
    , conditional_mixin_t<
        texture_target_traits<TargetV>::is_layered ||
        TargetV == TextureTarget::Texture1D ||
        TargetV == TextureTarget::Texture2D ||
        TargetV == TextureTarget::Texture2DMS,
        TextureDSAInterface_Queries_ViewLike_MinLayer<CRTP, TargetV>
    >
    // You can't view LOD texture with non-LOD texture and vice-versa.
    , conditional_mixin_t<
        texture_target_traits<TargetV>::has_lod,
        TextureDSAInterface_Queries_ViewLike_Levels<CRTP, TargetV>
    >
{};









template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_NumArrayElements {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:
    auto get_num_array_elements_impl(MipLevel level) const noexcept
        -> GLsizei
            requires tt::is_array
    {
        if constexpr (TargetV == TextureTarget::Texture1DArray) {
            GLsizei height;
            gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_HEIGHT, &height);
            return height;
        } else if constexpr (TargetV == TextureTarget::Texture2DArray || TargetV == TextureTarget::Texture2DMSArray) {
            GLsizei depth;
            gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_DEPTH,  &depth);
            return depth;
        } else if constexpr (TargetV == TextureTarget::CubemapArray) {
            GLsizei depth;
            gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_DEPTH,  &depth);
            assert((depth % 6) == 0 &&
                "It's 2AM, I've looked at the spec. The spec has words. Many. I'm pretty sure it said this shouldn't happen. Maybe.");
            return depth / 6;
        }
    }
public:

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_[HEIGHT|DEPTH]` and `level = 0`.
    auto get_num_array_elements() const noexcept
        -> GLsizei
            requires tt::is_array
    {
        return get_num_array_elements_impl(MipLevel{ 0 });
    }

};





template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_MultisampleParams {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_SAMPLES` and `level = 0`.
    auto get_num_samples() const noexcept
        -> NumSamples
            requires tt::is_multisample
    {
        GLsizei nsamples;
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_SAMPLES, &nsamples);
        return NumSamples{ nsamples };
    }

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_FIXED_SAMPLE_LOCATIONS` and `level = 0`.
    auto get_sample_locations() const noexcept
        -> SampleLocations
            requires tt::is_multisample
    {
        GLboolean fixed_sample_locations;
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_FIXED_SAMPLE_LOCATIONS, &fixed_sample_locations);
        return SampleLocations{ bool(fixed_sample_locations) };
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_InternalFormat {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Wraps `glGetTextureLevelParameteriv` with `pname = GL_TEXTURE_INTERNAL_FORMAT` and `level = 0`.
    auto get_internal_format() const noexcept
        -> InternalFormat
    {
        GLenum internal_format;
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
        return enum_cast<InternalFormat>(internal_format);
    }

};








template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_ImageFormatCompatibility {
    JOSH3D_TEXTURE_MIXIN_HEADER

    auto get_image_unit_format_compatibility() const noexcept
        -> ImageUnitFormatCompatibility
    {
        GLenum compat;
        gl::glGetTextureParameteriv(self_id(), gl::GL_IMAGE_FORMAT_COMPATIBILITY_TYPE, &compat);
        return enum_cast<ImageUnitFormatCompatibility>(compat);
    }

};








template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_Compressed {
    JOSH3D_TEXTURE_MIXIN_HEADER

    bool is_compressed() const noexcept {
        GLboolean compressed;
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_COMPRESSED, &compressed);
        return bool(compressed);
    }

    auto get_compressed_image_size_bytes(MipLevel level = MipLevel{ 0 }) const noexcept
        -> GLsizei
            requires tt::has_lod
    {
        GLsizei size_bytes;
        gl::glGetTextureLevelParameteriv(self_id(), level, gl::GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size_bytes);
        return size_bytes;
    }

    auto get_compressed_image_size_bytes() const noexcept
        -> GLsizei
            requires (!tt::has_lod)
    {
        GLsizei size_bytes;
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size_bytes);
        return size_bytes;
    }

};






template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_BufferTexture {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:
    // TODO: Is this actually supported though?
    GLuint get_attached_buffer_impl() const noexcept {
        GLenum buf; // enum because that's unsigned int.
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_BUFFER_DATA_STORE_BINDING, &buf);
        return enum_cast<GLuint>(buf);
    }
public:

    bool has_buffer_attached() const noexcept {
        return bool(get_attached_buffer_impl());
    }

    auto get_attached_buffer() const noexcept
        -> RawUntypedBuffer<GLMutable>
            requires mt::is_mutable
    {
        return RawUntypedBuffer<GLMutable>::from_id(get_attached_buffer_impl());
    }

    auto get_attached_buffer() const noexcept
        -> RawUntypedBuffer<GLConst>
            requires mt::is_const
    {
        return RawUntypedBuffer<GLConst>::from_id(get_attached_buffer_impl());
    }

    auto get_attached_buffer_size_bytes() const noexcept
        -> GLsizeiptr
    {
        GLsizei size;
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_BUFFER_SIZE, &size);
        return GLsizeiptr(size);
    }

    auto get_attached_buffer_offset_bytes() const noexcept
        -> GLintptr
    {
        GLint offset;
        gl::glGetTextureLevelParameteriv(self_id(), 0, gl::GL_TEXTURE_BUFFER_OFFSET, &offset);
        return GLintptr(offset);
    }


};






template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries_ComponentSizeType {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // TODO: Is this in bits?
    template<PixelComponent ComponentV>
    auto _get_component_size() const noexcept
        -> GLsizei
    {
        auto size = [&](GLenum pname) -> GLsizei {
            MipLevel level{ 0 };
            GLsizei size;
            gl::glGetTextureLevelParameteriv(
                self_id(), level, pname, &size
            );
            return size;
        };
        if constexpr      (ComponentV == PixelComponent::Red)            { return size(gl::GL_TEXTURE_RED_SIZE);     }
        else if constexpr (ComponentV == PixelComponent::Green)          { return size(gl::GL_TEXTURE_GREEN_SIZE);   }
        else if constexpr (ComponentV == PixelComponent::Blue)           { return size(gl::GL_TEXTURE_BLUE_SIZE);    }
        else if constexpr (ComponentV == PixelComponent::Alpha)          { return size(gl::GL_TEXTURE_ALPHA_SIZE);   }
        else if constexpr (ComponentV == PixelComponent::Depth)          { return size(gl::GL_TEXTURE_DEPTH_SIZE);   }
        else if constexpr (ComponentV == PixelComponent::Stencil)        { return size(gl::GL_TEXTURE_STENCIL_SIZE); }
        else if constexpr (ComponentV == PixelComponent::SharedExponent) { return size(gl::GL_TEXTURE_SHARED_SIZE);  }
        else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

    template<PixelComponent ComponentV>
    auto get_component_type() const noexcept
        -> PixelComponentType
            requires
                (ComponentV != PixelComponent::Stencil) &&
                (ComponentV != PixelComponent::SharedExponent)
    {
        auto type = [&](GLenum pname) -> PixelComponentType {
            MipLevel level{ 0 };
            GLenum type;
            gl::glGetTextureLevelParameteriv(
                self_id(), level, pname, &type
            );
            return enum_cast<PixelComponentType>(type);
        };
        if constexpr      (ComponentV == PixelComponent::Red)   { return type(gl::GL_TEXTURE_RED_SIZE);   }
        else if constexpr (ComponentV == PixelComponent::Green) { return type(gl::GL_TEXTURE_GREEN_SIZE); }
        else if constexpr (ComponentV == PixelComponent::Blue)  { return type(gl::GL_TEXTURE_BLUE_SIZE);  }
        else if constexpr (ComponentV == PixelComponent::Alpha) { return type(gl::GL_TEXTURE_ALPHA_SIZE); }
        else if constexpr (ComponentV == PixelComponent::Depth) { return type(gl::GL_TEXTURE_DEPTH_SIZE); }
    }

};









template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Queries
    : TextureDSAInterface_Queries_ResolutionAndExtent<CRTP, TargetV>
    , TextureDSAInterface_Queries_InternalFormat<CRTP, TargetV>
    , TextureDSAInterface_Queries_ComponentSizeType<CRTP, TargetV>
    , TextureDSAInterface_Queries_ViewLike<CRTP, TargetV>
    , conditional_mixin_t<
        texture_target_traits<TargetV>::is_array,
        TextureDSAInterface_Queries_NumArrayElements<CRTP, TargetV>
    >
    , conditional_mixin_t<
        texture_target_traits<TargetV>::is_multisample,
        TextureDSAInterface_Queries_MultisampleParams<CRTP, TargetV>
    >
    , TextureDSAInterface_Queries_ImageFormatCompatibility<CRTP, TargetV>
    , conditional_mixin_t<
        texture_target_traits<TargetV>::supports_compressed_internal_format,
        TextureDSAInterface_Queries_Compressed<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV == TextureTarget::TextureBuffer,
        TextureDSAInterface_Queries_BufferTexture<CRTP, TargetV>
    >
{};













template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_SamplerParameters_CompareMode {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Compare Func.

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_COMPARE_FUNC`.
    void set_sampler_compare_func(CompareOp compare_func) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_COMPARE_FUNC, enum_cast<GLenum>(compare_func));
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_COMPARE_FUNC`.
    auto get_sampler_compare_func() const noexcept
        -> CompareOp
    {
        GLenum op;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_COMPARE_FUNC, &op);
        return enum_cast<CompareOp>(op);
    }


    // Compare Mode.

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_COMPARE_MODE`.
    // Passes `GL_COMPARE_REF_TO_TEXTURE` if `enable_compare_mode` is `true`, `GL_NONE` otherwise.
    void set_sampler_compare_ref_depth_to_texture(bool enable_compare_mode) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameteri(
            self_id(), gl::GL_TEXTURE_COMPARE_MODE,
            enable_compare_mode ? gl::GL_COMPARE_REF_TO_TEXTURE : gl::GL_NONE
        );
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_COMPARE_MODE`.
    // Returns `true` if the result is `GL_COMPARE_REF_TO_TEXTURE`, `false` otherwise.
    bool get_sampler_compare_ref_depth_to_texture() const noexcept {
        GLenum mode;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_COMPARE_MODE, mode);
        return mode == gl::GL_COMPARE_REF_TO_TEXTURE;
    }

};


template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_SamplerParameters_LOD {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // LOD Bias.

    // Wraps `glTextureParameterf` with `pname = GL_TEXTURE_LOD_BIAS`.
    void set_sampler_lod_bias(GLfloat bias) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameterf(self_id(), gl::GL_TEXTURE_LOD_BIAS, bias);
    }

    // Wraps `glGetTextureParameterfv` with `pname = GL_TEXTURE_LOD_BIAS`.
    auto get_sampler_lod_bias() const noexcept
        -> GLfloat
    {
        GLfloat bias;
        gl::glGetTextureParameterfv(self_id(), gl::GL_TEXTURE_LOD_BIAS, &bias);
        return bias;
    }



    // Min/Max LOD.

    // Wraps `glTextureParameterf` with `pname = GL_TEXTURE_MIN_LOD`.
    void set_sampler_min_lod(GLfloat min_lod) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameterf(self_id(), gl::GL_TEXTURE_MIN_LOD, min_lod);
    }

    // Wraps `glTextureParameterf` with `pname = GL_TEXTURE_MAX_LOD`.
    void set_sampler_max_lod(GLfloat max_lod) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameterf(self_id(), gl::GL_TEXTURE_MAX_LOD, max_lod);
    }

    // Wraps `glTextureParameterf` with `pname = GL_TEXTURE_[MIN|MAX]_LOD`.
    void set_sampler_min_max_lod(GLfloat min_lod, GLfloat max_lod) const noexcept
        requires mt::is_mutable
    {
        set_sampler_min_lod(min_lod);
        set_sampler_max_lod(max_lod);
    }

    // Wraps `glGetTextureParameterfv` with `pname = GL_TEXTURE_MIN_LOD`.
    auto get_sampler_min_lod() const noexcept
        -> GLfloat
    {
        GLfloat min_lod;
        gl::glGetTextureParameterfv(self_id(), gl::GL_TEXTURE_MIN_LOD, &min_lod);
        return min_lod;
    }

    // Wraps `glGetTextureParameterfv` with `pname = GL_TEXTURE_MAX_LOD`.
    auto get_sampler_max_lod() const noexcept
        -> GLfloat
    {
        GLfloat max_lod;
        gl::glGetTextureParameterfv(self_id(), gl::GL_TEXTURE_MAX_LOD, &max_lod);
        return max_lod;
    }



    // Max Anisotropy.

    // Wraps `glTextureParameterf` with `pname = GL_TEXTURE_MAX_ANISOTROPY`.
    void set_sampler_max_anisotropy(GLfloat max_anisotropy) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameterf(self_id(), gl::GL_TEXTURE_MAX_ANISOTROPY, max_anisotropy);
    }

    // Wraps `glGetTextureParameterfv` with `pname = GL_TEXTURE_MAX_ANISOTROPY`.
    auto get_sampler_max_anisotropy() const noexcept
        -> GLfloat
    {
        GLfloat max_anisotropy;
        gl::glGetTextureParameterfv(self_id(), gl::GL_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
        return max_anisotropy;
    }


};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_SamplerParameters_MinMagFilters {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Min/Mag Filters.

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_MIN_FILTER`.
    void set_sampler_min_filter(MinFilter min_filter) const noexcept
        requires mt::is_mutable && tt::has_lod
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_MIN_FILTER, enum_cast<GLenum>(min_filter));
    }

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_MIN_FILTER`.
    void set_sampler_min_filter(MinFilterNoLOD min_filter) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_MIN_FILTER, enum_cast<GLenum>(min_filter));
    }

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_MAG_FILTER`.
    void set_sampler_mag_filter(MagFilter mag_filter) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_MAG_FILTER, enum_cast<GLenum>(mag_filter));
    }

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_[MIN|MAG]_FILTER`.
    void set_sampler_min_mag_filters(MinFilter min_filter, MagFilter mag_filter) const noexcept
        requires mt::is_mutable && tt::has_lod
    {
        set_sampler_min_filter(min_filter);
        set_sampler_mag_filter(mag_filter);
    }

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_[MIN|MAG]_FILTER`.
    void set_sampler_min_mag_filters(MinFilterNoLOD min_filter, MagFilter mag_filter) const noexcept
        requires mt::is_mutable
    {
        set_sampler_min_filter(min_filter);
        set_sampler_mag_filter(mag_filter);
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_MIN_FILTER`.
    auto get_sampler_min_filter() const noexcept
        -> MinFilter
            requires tt::has_lod
    {
        GLenum result;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_MIN_FILTER, &result);
        return enum_cast<MinFilter>(result);
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_MIN_FILTER`.
    auto get_sampler_min_filter() const noexcept
        -> MinFilterNoLOD
            requires (!tt::has_lod)
    {
        GLenum result;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_MIN_FILTER, &result);
        return enum_cast<MinFilterNoLOD>(result);
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_MAG_FILTER`.
    auto get_sampler_mag_filter() const noexcept
        -> MagFilter
    {
        GLenum result;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_MAG_FILTER, &result);
        return enum_cast<MagFilter>(result);
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_SamplerParameters_BorderColor {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Wraps `glTextureParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_unorm(GLfloat r, GLfloat g, GLfloat b, GLfloat a) const noexcept
        requires mt::is_mutable
    {
        GLfloat rgbaf[4]{ r, g, b, a };
        gl::glTextureParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbaf);
    }

    // Wraps `glTextureParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_unorm(RGBAUNorm rgba) const noexcept
        requires mt::is_mutable
    {
        set_sampler_border_color_unorm(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glTextureParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_snorm(GLint r, GLint g, GLint b, GLint a) const noexcept
        requires mt::is_mutable
    {
        GLint rgba_snorm[4]{ r, g, b, a };
        gl::glTextureParameteriv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba_snorm);
    }

    // Wraps `glTextureParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_snorm(RGBASNorm rgba) const noexcept
        requires mt::is_mutable
    {
        set_sampler_border_color_snorm(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glTextureParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_float(GLfloat r, GLfloat g, GLfloat b, GLfloat a) const noexcept
        requires mt::is_mutable
    {
        GLfloat rgbaf[4]{ r, g, b, a };
        gl::glTextureParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbaf);
    }

    // Wraps `glTextureParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_float(RGBAF rgba) const noexcept
        requires mt::is_mutable
    {
        set_sampler_border_color_float(rgba.r, rgba.g, rgba.b, rgba.a);
    }


    // Wraps `glTextureParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_integer(GLint r, GLint g, GLint b, GLint a) const noexcept
        requires mt::is_mutable
    {
        GLint rgbai[4]{ r, g, b, a };
        gl::glTextureParameterIiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbai);
    }

    // Wraps `glTextureParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_integer(RGBAI rgba) const noexcept
        requires mt::is_mutable
    {
        set_sampler_border_color_integer(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glTextureParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_unsigned_integer(GLuint r, GLuint g, GLuint b, GLuint a) const noexcept
        requires mt::is_mutable
    {
        GLuint rgbaui[4]{ r, g, b, a };
        gl::glTextureParameterIuiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbaui);
    }

    // Wraps `glTextureParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_sampler_border_color_unsigned_integer(RGBAUI rgba) const noexcept
        requires mt::is_mutable
    {
        set_sampler_border_color_unsigned_integer(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glGetTextureParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_sampler_border_color_unorm() const noexcept
        -> RGBAUNorm
    {
        GLfloat rgba[4];
        gl::glGetTextureParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_sampler_border_color_snorm() const noexcept
        -> RGBASNorm
    {
        GLint rgba[4];
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetTextureParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_sampler_border_color_float() const noexcept
        -> RGBAF
    {
        GLfloat rgba[4];
        gl::glGetTextureParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetTextureParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_sampler_border_color_integer() const noexcept
        -> RGBAI
    {
        GLint rgba[4];
        gl::glGetTextureParameterIiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetTextureParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_sampler_border_color_unsigned_integer() const noexcept
        -> RGBAUI
    {
        GLuint rgba[4];
        gl::glGetTextureParameterIuiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_SamplerParameters_Wrap {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Wrap.

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_WRAP_S`.
    void set_sampler_wrap_s(Wrap wrap_s) const noexcept
        requires mt::is_mutable && (tt::resolution_ndims >= 1)
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_WRAP_S, enum_cast<GLenum>(wrap_s));
    }

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_WRAP_T`.
    void set_sampler_wrap_t(Wrap wrap_t) const noexcept
        requires mt::is_mutable && (tt::resolution_ndims >= 2)
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_WRAP_T, enum_cast<GLenum>(wrap_t));
    }

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_WRAP_R`.
    void set_sampler_wrap_r(Wrap wrap_r) const noexcept
        requires mt::is_mutable && (tt::resolution_ndims >= 3)
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_WRAP_R, enum_cast<GLenum>(wrap_r));
    }

    // Wraps `glTextureParameteri` with `pname = GL_TEXTURE_WRAP_[S|T|R]`.
    void set_sampler_wrap_all(Wrap wrap_str) const noexcept
        requires mt::is_mutable
    {
        if constexpr        (tt::resolution_ndims == 1) {
            set_sampler_wrap_s(wrap_str);
        } else if constexpr (tt::resolution_ndims == 2) {
            set_sampler_wrap_s(wrap_str);
            set_sampler_wrap_t(wrap_str);
        } else if constexpr (tt::resolution_ndims == 3) {
            set_sampler_wrap_s(wrap_str);
            set_sampler_wrap_t(wrap_str);
            set_sampler_wrap_r(wrap_str);
        }
        else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_WRAP_S`.
    Wrap get_sampler_wrap_s() const noexcept
        requires (tt::resolution_ndims >= 1)
    {
        GLenum wrap;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_WRAP_S, &wrap);
        return enum_cast<Wrap>(wrap);
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_WRAP_T`.
    Wrap get_sampler_wrap_t() const noexcept
        requires (tt::resolution_ndims >= 2)
    {
        GLenum wrap;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_WRAP_T, &wrap);
        return enum_cast<Wrap>(wrap);
    }

    // Wraps `glGetTextureParameteriv` with `pname = GL_TEXTURE_WRAP_R`.
    Wrap get_sampler_wrap_r() const noexcept
        requires (tt::resolution_ndims >= 3)
    {
        GLenum wrap;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_WRAP_R, &wrap);
        return enum_cast<Wrap>(wrap);
    }

};









template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_SamplerParameters
    : TextureDSAInterface_SamplerParameters_CompareMode<CRTP, TargetV>
    , conditional_mixin_t<
        texture_target_traits<TargetV>::has_lod,
        TextureDSAInterface_SamplerParameters_LOD<CRTP, TargetV>
    >
    , TextureDSAInterface_SamplerParameters_MinMagFilters<CRTP, TargetV>
    , TextureDSAInterface_SamplerParameters_BorderColor<CRTP, TargetV>
    , TextureDSAInterface_SamplerParameters_Wrap<CRTP, TargetV>
{};










template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_TextureParameters_Swizzle {
    JOSH3D_TEXTURE_MIXIN_HEADER

    void set_swizzle_rgba(
        Swizzle red, Swizzle green, Swizzle based, Swizzle alpha) const noexcept
            requires mt::is_mutable
    {
        GLenum params[4]{
            enum_cast<GLenum>(red),   enum_cast<GLenum>(green),
            enum_cast<GLenum>(based), enum_cast<GLenum>(alpha),
        };
        gl::glTextureParameteriv(self_id(), gl::GL_TEXTURE_SWIZZLE_RGBA, params);
    }

    void set_swizzle_rgba(SwizzleRGBA swizzle_rgba) const noexcept
        requires mt::is_mutable
    {
        set_swizzle_rgba(swizzle_rgba.r, swizzle_rgba.g, swizzle_rgba.b, swizzle_rgba.a);
    }

    auto get_swizzle_rgba() const noexcept
        -> SwizzleRGBA
    {
        GLenum params[4];
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_SWIZZLE_RGBA, params);
        return SwizzleRGBA{
            enum_cast<Swizzle>(params[0]),
            enum_cast<Swizzle>(params[1]),
            enum_cast<Swizzle>(params[2]),
            enum_cast<Swizzle>(params[3]),
        };
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_TextureParameters_BaseAndMaxLODs {
    JOSH3D_TEXTURE_MIXIN_HEADER

    void set_base_lod(MipLevel level) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_BASE_LEVEL, level);
    }

    auto get_base_lod() const noexcept
        -> MipLevel
    {
        GLint level;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_BASE_LEVEL, &level);
        return MipLevel{ level };
    }


    void set_max_lod(MipLevel max_level) const noexcept
        requires mt::is_mutable
    {
        gl::glTextureParameteri(self_id(), gl::GL_TEXTURE_MAX_LEVEL, max_level);
    }

    auto get_max_lod() const noexcept
        -> MipLevel
    {
        GLint max_level;
        gl::glGetTextureParameteriv(self_id(), gl::GL_TEXTURE_MAX_LOD, max_level);
        return MipLevel{ max_level };
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_TextureParameters_StencilTexturing {
    JOSH3D_TEXTURE_MIXIN_HEADER

    void set_depth_stencil_sampling_target(DepthStencilTarget target_to_sample) const noexcept
        requires mt::is_mutable // Is this a good idea?
    {
        gl::glTextureParameteri(
            self_id(), gl::GL_DEPTH_STENCIL_TEXTURE_MODE, enum_cast<GLenum>(target_to_sample)
        );
    }

    auto get_depth_stencil_sampling_target() const noexcept
        -> DepthStencilTarget
    {
        GLenum target;
        gl::glGetTextureParameteriv(
            self_id(), gl::GL_DEPTH_STENCIL_TEXTURE_MODE, &target
        );
        return enum_cast<DepthStencilTarget>(target);
    }

};











template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_TextureParameters
    : TextureDSAInterface_TextureParameters_Swizzle<CRTP, TargetV>
    , conditional_mixin_t<
        texture_target_traits<TargetV>::has_lod,
        TextureDSAInterface_TextureParameters_BaseAndMaxLODs<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer,
        TextureDSAInterface_TextureParameters_StencilTexturing<CRTP, TargetV>
    >
 {};













template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Bind_ToTextureUnit {
    JOSH3D_TEXTURE_MIXIN_HEADER

    void bind_to_texture_unit(GLuint unit_index) const noexcept {
        gl::glBindTextureUnit(unit_index, self_id());
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Bind_ToImageUnit {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:
    void bind_to_image_unit_impl(
        GLuint index, ImageUnitFormat format, GLenum access, GLint level) const noexcept
    {
        gl::glBindImageTexture(
            index, self_id(), level, gl::GL_TRUE, 0, access, enum_cast<GLenum>(format)
        );
    }
public:

    void bind_to_readonly_image_unit(
        ImageUnitFormat format, GLuint unit_index, MipLevel level = MipLevel{ 0 }) const noexcept
            requires tt::has_lod
    {
        bind_to_image_unit_impl(unit_index, format, gl::GL_READ_ONLY, level);
    }

    void bind_to_readonly_image_unit(
        ImageUnitFormat format, GLuint unit_index) const noexcept
            requires (!tt::has_lod)
    {
        bind_to_image_unit_impl(unit_index, format, gl::GL_READ_ONLY, 0);
    }




    void bind_to_writeonly_image_unit(
        ImageUnitFormat format, GLuint unit_index, MipLevel level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        bind_to_image_unit_impl(unit_index, format, gl::GL_WRITE_ONLY, level);
    }

    void bind_to_writeonly_image_unit(
        ImageUnitFormat format, GLuint unit_index) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        bind_to_image_unit_impl(unit_index, format, gl::GL_WRITE_ONLY, 0);
    }




    void bind_to_readwrite_image_unit(
        ImageUnitFormat format, GLuint unit_index, MipLevel level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        bind_to_image_unit_impl(unit_index, format, gl::GL_READ_WRITE, level);
    }

    void bind_to_readwrite_image_unit(
        ImageUnitFormat format, GLuint unit_index) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        bind_to_image_unit_impl(unit_index, format, gl::GL_READ_WRITE, 0);
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Bind_ToImageUnitLayered {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:
    void bind_layer_to_image_unit_impl(
        GLuint index, ImageUnitFormat format, GLenum access, GLint layer, GLint level) const noexcept
    {
        gl::glBindImageTexture(
            index, self_id(), level, gl::GL_FALSE, layer, access, enum_cast<GLenum>(format)
        );
    }
public:

    void bind_layer_to_readonly_image_unit(
        Layer layer, ImageUnitFormat format, GLuint unit_index, MipLevel level = MipLevel{ 0 }) const noexcept
            requires tt::is_layered && tt::has_lod
    {
        bind_layer_to_image_unit_impl(unit_index, format, gl::GL_READ_ONLY, layer, level);
    }

    void bind_layer_to_readonly_image_unit(
        Layer layer, ImageUnitFormat format, GLuint unit_index) const noexcept
            requires tt::is_layered && (!tt::has_lod)
    {
        bind_layer_to_image_unit_impl(unit_index, format, gl::GL_READ_ONLY, layer, 0);
    }




    void bind_layer_to_writeonly_image_unit(
        Layer layer, ImageUnitFormat format, GLuint unit_index, MipLevel level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::is_layered && tt::has_lod
    {
        bind_layer_to_image_unit_impl(unit_index, format, gl::GL_WRITE_ONLY, layer, level);
    }

    void bind_layer_to_writeonly_image_unit(
        Layer layer, ImageUnitFormat format, GLuint unit_index) const noexcept
            requires mt::is_mutable && tt::is_layered && (!tt::has_lod)
    {
        bind_layer_to_image_unit_impl(unit_index, format, gl::GL_WRITE_ONLY, layer, 0);
    }




    void bind_layer_to_readwrite_image_unit(
        Layer layer, ImageUnitFormat format, GLuint unit_index, MipLevel level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::is_layered && tt::has_lod
    {
        bind_layer_to_image_unit_impl(unit_index, format, gl::GL_READ_WRITE, layer, level);
    }

    void bind_layer_to_readwrite_image_unit(
        Layer layer, ImageUnitFormat format, GLuint unit_index) const noexcept
            requires mt::is_mutable && tt::is_layered && (!tt::has_lod)
    {
        bind_layer_to_image_unit_impl(unit_index, format, gl::GL_READ_WRITE, layer, 0);
    }

};





template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_Bind
    : TextureDSAInterface_Bind_ToTextureUnit<CRTP, TargetV>
    , TextureDSAInterface_Bind_ToImageUnit<CRTP, TargetV>
    , conditional_mixin_t<
        texture_target_traits<TargetV>::is_layered,
        TextureDSAInterface_Bind_ToImageUnitLayered<CRTP, TargetV>
    >
{};








inline void texture_storage_1d(
    GLuint id, const Size1I& size,
    InternalFormat internal_format, GLsizei levels) noexcept
{
    gl::glTextureStorage1D(
        id, levels, enum_cast<GLenum>(internal_format), size
    );
}

inline void texture_storage_2d(
    GLuint id, const Size2I& size,
    InternalFormat internal_format, GLsizei levels) noexcept
{
    gl::glTextureStorage2D(
        id, levels, enum_cast<GLenum>(internal_format), size.width, size.height
    );
}

inline void texture_storage_2d_ms(
    GLuint id, const Size2I& size,
    InternalFormat internal_format,
    NumSamples num_samples, SampleLocations sample_locations) noexcept
{
    gl::glTextureStorage2DMultisample(
        id, num_samples, enum_cast<GLenum>(internal_format),
        size.width, size.height, enum_cast<bool>(sample_locations)
    );
}

inline void texture_storage_3d(
    GLuint id, const Size3I& size,
    InternalFormat internal_format, GLsizei levels) noexcept
{
    gl::glTextureStorage3D(
        id, levels, enum_cast<GLenum>(internal_format), size.width, size.height, size.depth
    );
}

inline void texture_storage_3d_ms(
    GLuint id, const Size3I& size,
    InternalFormat internal_format,
    NumSamples num_samples, SampleLocations sample_locations) noexcept
{
    gl::glTextureStorage3DMultisample(
        id, num_samples, enum_cast<GLenum>(internal_format),
        size.width, size.height, size.depth, enum_cast<bool>(sample_locations)
    );
}







template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_AllocateStorage {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Overload for `Texture[1|2|3]D`, `Cubemap`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        InternalFormat             internal_format,
        NumLevels                  num_levels = NumLevels{ 1 }) const noexcept
            requires mt::is_mutable && tt::has_lod && (!tt::is_array)
    {
        if constexpr (tt::resolution_ndims == 1) {
            texture_storage_1d(self_id(), resolution, internal_format, num_levels);
        } else if constexpr (tt::resolution_ndims == 2) {
            texture_storage_2d(self_id(), resolution, internal_format, num_levels);
        } else if constexpr (tt::resolution_ndims == 3) {
            texture_storage_3d(self_id(), resolution, internal_format, num_levels);
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }



    // Overload for `TextureRectangle`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        InternalFormat             internal_format) const noexcept
            requires mt::is_mutable && (!tt::has_lod) && (!tt::is_array) && (!tt::is_multisample)
    {
        if constexpr (tt::resolution_ndims == 1) {
            texture_storage_1d(self_id(), resolution, internal_format, 1);
        } else if constexpr (tt::resolution_ndims == 2) {
            texture_storage_2d(self_id(), resolution, internal_format, 1);
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

    // Overload for `Texture[1|2]DArray`, `CubemapArray`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        GLsizei                    num_array_elements,
        InternalFormat             internal_format,
        NumLevels                  num_levels = NumLevels{ 1 }) const noexcept
            requires mt::is_mutable && tt::has_lod && tt::is_array
    {
        if constexpr (tt::resolution_ndims == 1) {
            texture_storage_2d(self_id(), Size2I{ resolution, num_array_elements }, internal_format, num_levels);
        } else if constexpr (tt::resolution_ndims == 2) {
            if constexpr (TargetV == TextureTarget::CubemapArray) {
                texture_storage_3d(self_id(), Size3I{ resolution, 6 * num_array_elements }, internal_format, num_levels);
            } else {
                texture_storage_3d(self_id(), Size3I{ resolution, num_array_elements }, internal_format, num_levels);
            }
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

    // Overload for `Texture2DMS`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        InternalFormat             internal_format,
        NumSamples                 num_samples      = NumSamples{ 1 },
        SampleLocations            sample_locations = SampleLocations::NotFixed) const noexcept
            requires mt::is_mutable && tt::is_multisample && (!tt::is_array)
    {
        if constexpr (tt::resolution_ndims == 2) {
            texture_storage_2d_ms(self_id(), resolution, internal_format, num_samples, sample_locations);
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

    // Overload for `Texture2DMSArray`.
    void allocate_storage(
        const tt::resolution_type& resolution,
        GLsizei                    num_array_elements,
        InternalFormat             internal_format,
        NumSamples                 num_samples      = NumSamples{ 1 },
        SampleLocations            sample_locations = SampleLocations::NotFixed) const noexcept
            requires mt::is_mutable && tt::is_multisample && tt::is_array
    {
        if constexpr (tt::resolution_ndims == 2) {
            texture_storage_3d_ms(self_id(), Size3I{ resolution, num_array_elements }, internal_format, num_samples, sample_locations);
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }


};







inline void texture_sub_image_1d(
    GLuint id, const Index1I& offset, const Size1I& size,
    PixelDataFormat format, PixelDataType type, const void* data, GLint mip_level) noexcept
{
    gl::glTextureSubImage1D(
        id, mip_level, offset, size,
        enum_cast<GLenum>(format), enum_cast<GLenum>(type), data
    );
}

inline void texture_sub_image_2d(
    GLuint id, const Index2I& offset, const Size2I& size,
    PixelDataFormat format, PixelDataType type, const void* data, GLint mip_level) noexcept
{
    gl::glTextureSubImage2D(
        id, mip_level, offset.x, offset.y, size.width, size.height,
        enum_cast<GLenum>(format), enum_cast<GLenum>(type), data
    );
}

inline void texture_sub_image_3d(
    GLuint id, const Index3I& offset, const Size3I& size,
    PixelDataFormat format, PixelDataType type, const void* data, GLint mip_level) noexcept
{
    gl::glTextureSubImage3D(
        id, mip_level, offset.x, offset.y, offset.z,
        size.width, size.height, size.depth,
        enum_cast<GLenum>(format), enum_cast<GLenum>(type), data
    );
}




inline GLenum best_unpack_format(
    GLenum target, GLenum internal_format) noexcept
{
    GLint format;
    gl::glGetInternalformativ(
        target, internal_format,
        gl::GL_TEXTURE_IMAGE_FORMAT,
        1, &format
    );
    return static_cast<GLenum>(format);
}

inline GLenum best_unpack_type(
    GLenum target, GLenum internal_format) noexcept
{
    switch (internal_format) {
        using enum GLenum;
        // * swearing *
        case GL_DEPTH24_STENCIL8:
            return GL_UNSIGNED_INT_24_8;
        case GL_DEPTH32F_STENCIL8:
            return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        default:
            GLint type;
            gl::glGetInternalformativ(
                target, internal_format,
                gl::GL_TEXTURE_IMAGE_TYPE,
                1, &type
            );
            return static_cast<GLenum>(type);
    }
}








template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_Upload {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:

    void upload_image_region_impl(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data,
        GLint                  mip_level) const noexcept
            requires mt::is_mutable
    {
        if constexpr        (tt::region_ndims == 1) {
            texture_sub_image_1d(self_id(), region.offset, region.extent, format, type, data, mip_level);
        } else if constexpr (tt::region_ndims == 2) {
            texture_sub_image_2d(self_id(), region.offset, region.extent, format, type, data, mip_level);
        } else if constexpr (tt::region_ndims == 3) {
            texture_sub_image_3d(self_id(), region.offset, region.extent, format, type, data, mip_level);
        } else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

public:

    void upload_image_region(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        upload_image_region_impl(region, format, type, data, level);
    }

    void upload_image_region(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        upload_image_region_impl(region, format, type, data, MipLevel{ 0 });
    }

    template<specifies_pixel_pack_traits PixelT>
    void upload_image_region(
        const tt::region_type& region,
        const PixelT*          data,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        using pptr = pixel_pack_traits<PixelT>;
        upload_image_region_impl(region, pptr::format, pptr::type, data, level);
    }

    template<specifies_pixel_pack_traits PixelT>
    void upload_image_region(
        const tt::region_type& region,
        const PixelT*          data) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        using pptr = pixel_pack_traits<PixelT>;
        upload_image_region_impl(region, pptr::format, pptr::type, data, MipLevel{ 0 });
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_Download {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:

    void download_image_region_into_impl(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        std::span<GLubyte>     dst_buf,
        MipLevel               level) const noexcept
    {
        auto download = [&, this] (
            const Index3I& offset,
            const Size3I&  extent)
        {
            gl::glGetTextureSubImage(
                self_id(), level,
                offset.x,     offset.y,      offset.z,
                extent.width, extent.height, extent.depth,
                enum_cast<GLenum>(format), enum_cast<GLenum>(type),
                dst_buf.size(),
                dst_buf.data()
            );
        };
        auto& offset = region.offset;
        auto& extent = region.extent;
        if constexpr      (tt::region_ndims == 1) { download({ offset, 0, 0 }, { extent, 1, 1 }); }
        else if constexpr (tt::region_ndims == 2) { download({ offset,    0 }, { extent,    1 }); }
        else if constexpr (tt::region_ndims == 3) { download({ offset       }, { extent       }); }
        else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

public:

    void download_image_region_into(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        std::span<GLubyte>     dst_buf,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires tt::has_lod
    {
        download_image_region_into_impl(region, format, type, dst_buf, level);
    }

    void download_image_region_into(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        std::span<GLubyte>     dst_buf) const noexcept
            requires (!tt::has_lod)
    {
        download_image_region_into_impl(region, format, type, dst_buf, MipLevel{ 0 });
    }

    template<specifies_pixel_pack_traits PixelT>
    void download_image_region_into(
        const tt::region_type& region,
        std::span<PixelT>      dst_buf,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires tt::has_lod
    {
        using pptr = pixel_pack_traits<PixelT>;
        download_image_region_into_impl(region, pptr::format, pptr::type, dst_buf, level);
    }

    template<specifies_pixel_pack_traits PixelT>
    void download_image_region_into(
        const tt::region_type& region,
        std::span<PixelT>      dst_buf) const noexcept
            requires (!tt::has_lod)
    {
        using pptr = pixel_pack_traits<PixelT>;
        download_image_region_into_impl(region, pptr::format, pptr::type, dst_buf, MipLevel{ 0 });
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_Copy {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:

    // The interpretation of the name depends on the value of the corresponding target parameter.
    // If target is GL_RENDERBUFFER, the name is interpreted as the name of a renderbuffer object.
    // If the target parameter is a texture target, the name is interpreted as a texture object.
    // All non-proxy texture targets are accepted, with the exception of GL_TEXTURE_BUFFER and the cubemap face selectors.
    template<typename DstTextureT>
        requires
            mutability_traits<DstTextureT>::is_mutable &&
            (of_kind<DstTextureT, GLKind::Texture> || of_kind<DstTextureT, GLKind::Renderbuffer>)
    void copy_image_region_to_impl(
        const tt::offset_type&                          src_offset,
        // We  restrict the dimensionality of the souce *Extent* type,
        // for cases where src_ndims > dst_ndims.
        //
        // For example, if the src is Texture3D and dst is Texture2D, then
        // the extent_type is restricted to Size2I, because copying full 3 dimensions
        // does not make sense when the destination is 2D.
        //
        // Also `cmp_less_equal` because the fucking `<=` breaks my poor linter.
        //
        // I'm so sorry for this ugliness...
        const std::conditional_t<(std::cmp_less_equal(tt::region_ndims, texture_traits<DstTextureT>::region_ndims)),
            typename tt::extent_type, typename texture_traits<DstTextureT>::extent_type>&
                                                        src_extent,
        const DstTextureT&                              dst_texture,
        const texture_traits<DstTextureT>::offset_type& dst_offset,
        MipLevel                                        src_level,
        MipLevel                                        dst_level) const noexcept
    {
        auto copy = [&, this] (
            const Offset3I& src_offset,
            const Extent3I& src_extent,
            const Offset3I& dst_offset)
        {
            gl::glCopyImageSubData(
                self_id(), static_cast<GLenum>(TargetV), src_level,
                src_offset.x, src_offset.y, src_offset.z,
                dst_texture.id(), static_cast<GLenum>(DstTextureT::target_type), dst_level,
                dst_offset.x, dst_offset.y, dst_offset.z,
                src_extent.width, src_extent.height, src_extent.depth
            );
        };
        using dtt = texture_traits<DstTextureT>;
        const auto& sof = src_offset;
        const auto& sex = src_extent;
        const auto& dof = dst_offset;
        constexpr auto sdims =  tt::region_ndims; // src
        constexpr auto ddims = dtt::region_ndims; // dst
        //                                                              restricted by ddims
        //                                                               when ddims < sdims
        if constexpr      (sdims == 1 && ddims == 1) { copy({ sof, 0, 0 }, { sex, 1, 1 }, { dof, 0, 0 }); }
        else if constexpr (sdims == 1 && ddims == 2) { copy({ sof, 0, 0 }, { sex, 1, 1 }, { dof,    0 }); }
        else if constexpr (sdims == 1 && ddims == 3) { copy({ sof, 0, 0 }, { sex, 1, 1 }, { dof       }); }
        else if constexpr (sdims == 2 && ddims == 1) { copy({ sof,    0 }, { sex, 1, 1 }, { dof, 0, 0 }); }
        else if constexpr (sdims == 2 && ddims == 2) { copy({ sof,    0 }, { sex,    1 }, { dof,    0 }); }
        else if constexpr (sdims == 2 && ddims == 3) { copy({ sof,    0 }, { sex,    1 }, { dof       }); }
        else if constexpr (sdims == 3 && ddims == 1) { copy({ sof       }, { sex, 1, 1 }, { dof, 0, 0 }); }
        else if constexpr (sdims == 3 && ddims == 2) { copy({ sof       }, { sex,    1 }, { dof,    0 }); }
        else if constexpr (sdims == 3 && ddims == 3) { copy({ sof       }, { sex       }, { dof       }); }
        else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }

public:

    /*
    Compatible internal formats for copying between compressed and
    uncompressed internal formats with CopyImageSubData.
    Formats with the same block size can be copied between each other.

    - 128-bit blocks:
    Uncompressed:
    -- RGBA32UI,
    -- RGBA32I,
    -- RGBA32F.
    Compatible Compressed:
    -- COMPRESSED_RG_RGTC2,
    -- COMPRESSED_SIGNED_RG_RGTC2,
    -- COMPRESSED_RGBA_BPTC_UNORM,
    -- COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
    -- COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
    -- COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT.

    - 64-bit blocks:
    Uncompressed:
    -- RGBA16F,
    -- RG32F,
    -- RGBA16UI,
    -- RG32UI,
    -- RGBA16I,
    -- RG32I,
    -- RGBA16,
    -- RGBA16_SNORM.
    Compatible Compressed:
    -- COMPRESSED_RED_RGTC1,
    -- COMPRESSED_SIGNED_RED_RGTC1.
    */


    template<typename DstTextureT>
        requires
            mutability_traits<DstTextureT>::is_mutable &&
            texture_traits<DstTextureT>::has_lod &&
            (of_kind<DstTextureT, GLKind::Texture> || of_kind<DstTextureT, GLKind::Renderbuffer>)
    void copy_image_region_to(
        const tt::offset_type&                          src_offset,
        const std::conditional_t<(std::cmp_less_equal(tt::region_ndims, texture_traits<DstTextureT>::region_ndims)),
            typename tt::extent_type, typename texture_traits<DstTextureT>::extent_type>&
                                                        src_extent,
        const DstTextureT&                              dst_texture,
        const texture_traits<DstTextureT>::offset_type& dst_offset,
        MipLevel                                        src_level = MipLevel{ 0 },
        MipLevel                                        dst_level = MipLevel{ 0 }) const noexcept
            requires tt::has_lod
    {
        copy_image_region_to_impl<DstTextureT>(src_offset, src_extent, dst_texture, dst_offset, src_level, dst_level);
    }

    // Src LOD, Dst No LOD.
    template<typename DstTextureT>
        requires
            mutability_traits<DstTextureT>::is_mutable &&
            (!texture_traits<DstTextureT>::has_lod) &&
            (of_kind<DstTextureT, GLKind::Texture> || of_kind<DstTextureT, GLKind::Renderbuffer>)
    void copy_image_region_to(
        const tt::offset_type&                          src_offset,
        const std::conditional_t<(std::cmp_less_equal(tt::region_ndims, texture_traits<DstTextureT>::region_ndims)),
            typename tt::extent_type, typename texture_traits<DstTextureT>::extent_type>&
                                                        src_extent,
        const DstTextureT&                              dst_texture,
        const texture_traits<DstTextureT>::offset_type& dst_offset,
        MipLevel                                        src_level = MipLevel{ 0 }) const noexcept
            requires tt::has_lod
    {
        copy_image_region_to_impl<DstTextureT>(src_offset, src_extent, dst_texture, dst_offset, src_level, MipLevel{ 0 });
    }

    // Src No LOD, Dst LOD.
    template<typename DstTextureT>
        requires
            mutability_traits<DstTextureT>::is_mutable &&
            texture_traits<DstTextureT>::has_lod &&
            (of_kind<DstTextureT, GLKind::Texture> || of_kind<DstTextureT, GLKind::Renderbuffer>)
    void copy_image_region_to(
        const tt::offset_type&                          src_offset,
        const std::conditional_t<(std::cmp_less_equal(tt::region_ndims, texture_traits<DstTextureT>::region_ndims)),
            typename tt::extent_type, typename texture_traits<DstTextureT>::extent_type>&
                                                        src_extent,
        const DstTextureT&                              dst_texture,
        const texture_traits<DstTextureT>::offset_type& dst_offset,
        MipLevel                                        dst_level = MipLevel{ 0 }) const noexcept
            requires (!tt::has_lod)
    {
        copy_image_region_to_impl<DstTextureT>(src_offset, src_extent, dst_texture, dst_offset, MipLevel{ 0 }, dst_level);
    }

    // Src No LOD, Dst No LOD.
    template<typename DstTextureT>
        requires
            mutability_traits<DstTextureT>::is_mutable &&
            (!texture_traits<DstTextureT>::has_lod) &&
            (of_kind<DstTextureT, GLKind::Texture> || of_kind<DstTextureT, GLKind::Renderbuffer>)
    void copy_image_region_to(
        const tt::offset_type&                          src_offset,
        const std::conditional_t<(std::cmp_less_equal(tt::region_ndims, texture_traits<DstTextureT>::region_ndims)),
            typename tt::extent_type, typename texture_traits<DstTextureT>::extent_type>&
                                                        src_extent,
        const DstTextureT&                              dst_texture,
        const texture_traits<DstTextureT>::offset_type& dst_offset) const noexcept
            requires (!tt::has_lod)
    {
        copy_image_region_to_impl<DstTextureT>(src_offset, src_extent, dst_texture, dst_offset, MipLevel{ 0 }, MipLevel{ 0 });
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_Fill {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:
    void fill_image_impl(
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data,
        MipLevel               level) const noexcept
    {
        gl::glClearTexImage(
            self_id(), level,
            enum_cast<GLenum>(format), enum_cast<GLenum>(type), data
        );
    }
public:

    void fill_image(
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        fill_image_impl(format, type, data, level);
    }

    void fill_image(
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        fill_image_impl(format, type, data, MipLevel{ 0 });
    }

    template<specifies_pixel_pack_traits PixelT>
    void fill_image(
        const PixelT& pixel_value,
        MipLevel      level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        using pptr = pixel_pack_traits<PixelT>;
        fill_image_impl(pptr::format, pptr::type, &pixel_value, level);
    }

    template<specifies_pixel_pack_traits PixelT>
    void fill_image(
        const PixelT& pixel_value) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        using pptr = pixel_pack_traits<PixelT>;
        fill_image_impl(pptr::format, pptr::type, &pixel_value, MipLevel{ 0 });
    }






private:
    void fill_image_region_impl(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data,
        MipLevel               level) const noexcept
    {
        auto fill = [&, this] (
            const Index3I& offset,
            const Size3I&  extent)
        {
            gl::glClearTexSubImage(
                self_id(), level,
                offset.x,     offset.y,      offset.z,
                extent.width, extent.height, extent.depth,
                enum_cast<GLenum>(format), enum_cast<GLenum>(type), data
            );
        };
        auto& offset = region.offset;
        auto& extent = region.extent;
        if constexpr      (tt::region_ndims == 1) { fill({ offset, 0, 0 }, { extent, 1, 1 }); }
        else if constexpr (tt::region_ndims == 2) { fill({ offset,    0 }, { extent,    1 }); }
        else if constexpr (tt::region_ndims == 3) { fill({ offset       }, { extent       }); }
        else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }
public:

    void fill_image_region(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        fill_image_region_impl(region, format, type, data, level);
    }

    void fill_image_region(
        const tt::region_type& region,
        PixelDataFormat        format,
        PixelDataType          type,
        const void*            data) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        fill_image_region_impl(region, format, type, data, MipLevel{ 0 });
    }

    template<specifies_pixel_pack_traits PixelT>
    void fill_image_region(
        const tt::region_type& region,
        const PixelT&          pixel_value,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        using pptr = pixel_pack_traits<PixelT>;
        fill_image_region_impl(region, pptr::format, pptr::type, &pixel_value, level);
    }

    template<specifies_pixel_pack_traits PixelT>
    void fill_image_region(
        const tt::region_type& region,
        const PixelT&          pixel_value) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        using pptr = pixel_pack_traits<PixelT>;
        fill_image_region_impl(region, pptr::format, pptr::type, &pixel_value, MipLevel{ 0 });
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_Clear {
    JOSH3D_TEXTURE_MIXIN_HEADER
private:
    void clear_image_impl(MipLevel level) const noexcept {
        // This is one of those functions that requires you to specify *correct* type and format
        // even though there's no data to unpack and the pointer is NULL. Insane.
        InternalFormat internal_format = this->get_internal_format(level);
        GLenum format = best_unpack_format(enum_cast<GLenum>(TargetV), enum_cast<GLenum>(internal_format));
        GLenum type   = best_unpack_type  (enum_cast<GLenum>(TargetV), enum_cast<GLenum>(internal_format));
        gl::glClearTexImage(
            self_id(), level,
            format, type, nullptr
        );
    }
public:

    void clear_image(MipLevel level = MipLevel{ 0 }) const noexcept
        requires mt::is_mutable && tt::has_lod
    {
        clear_image_impl(level);
    }

    void clear_image() const noexcept
        requires mt::is_mutable && (!tt::has_lod)
    {
        clear_image_impl(MipLevel{ 0 });
    }




private:
    void clear_image_region_impl(
        const tt::region_type& region,
        MipLevel               level) const noexcept
    {
        // This is one of those functions that requires you to specify *correct* type and format
        // even though there's no data to unpack and the pointer is NULL. Insane.
        InternalFormat internal_format = this->get_internal_format(level);
        GLenum format = best_unpack_format(enum_cast<GLenum>(TargetV), enum_cast<GLenum>(internal_format));
        GLenum type   = best_unpack_type  (enum_cast<GLenum>(TargetV), enum_cast<GLenum>(internal_format));

        auto clear = [&, this] (
            const Index3I& offset,
            const Size3I&  extent)
        {
            gl::glClearTexSubImage(
                self_id(), level,
                offset.x, offset.y, offset.z,
                extent.width, extent.height, extent.depth,
                format, type, nullptr
            );
        };
        auto& [offset, extent] = region;
        if constexpr      (tt::region_ndims == 1) { clear({ offset, 0, 0 }, { extent, 1, 1 }); }
        else if constexpr (tt::region_ndims == 2) { clear({ offset,    0 }, { extent,    1 }); }
        else if constexpr (tt::region_ndims == 3) { clear({ offset       }, { extent       }); }
        else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }
public:

    void clear_image_region(
        const tt::region_type& region,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        clear_image_region_impl(region, level);
    }

    void clear_image_region(
        const tt::region_type& region) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        clear_image_region_impl(region, MipLevel{ 0 });
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_Invalidate {
    JOSH3D_TEXTURE_MIXIN_HEADER

    void invalidate_image(MipLevel level = MipLevel{ 0 }) const noexcept
        requires mt::is_mutable && tt::has_lod
    {
        gl::glInvalidateTexImage(self_id(), level);
    }

    void invalidate_image() const noexcept
        requires mt::is_mutable && (!tt::has_lod)
    {
        gl::glInvalidateTexImage(self_id(), MipLevel{ 0 });
    }




private:
    void invalidate_image_region_impl(
        const tt::region_type& region,
        MipLevel               level) const noexcept
    {
        auto invalidate = [&, this] (
            const Index3I& offset,
            const Size3I&  extent)
        {
            gl::glInvalidateTexSubImage(
                self_id(), level,
                offset.x,     offset.y,      offset.z,
                extent.width, extent.height, extent.depth
            );
        };
        auto& [offset, extent] = region;
        if constexpr      (tt::region_ndims == 1) { invalidate({ offset, 0, 0 }, { extent, 1, 1 }); }
        else if constexpr (tt::region_ndims == 2) { invalidate({ offset,    0 }, { extent,    1 }); }
        else if constexpr (tt::region_ndims == 3) { invalidate({ offset       }, { extent       }); }
        else { JOSH3D_STATIC_ASSERT_FALSE(tt); }
    }
public:

    void invalidate_image_region(
        const tt::region_type& region,
        MipLevel               level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable && tt::has_lod
    {
        invalidate_image_region_impl(region, level);
    }

    void invalidate_image_region(
        const tt::region_type& region) const noexcept
            requires mt::is_mutable && (!tt::has_lod)
    {
        invalidate_image_region_impl(region, MipLevel{ 0 });
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_UploadCompressed {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // TODO:
    void _upload_compressed_image_region() const noexcept {}

};





template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_DownloadCompressed {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // TODO:
    // void _download_compressed_image_into() const noexcept {}
    void _download_compressed_image_region_into() const noexcept {}

};





template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_UploadFromReadFramebuffer {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // TODO:
    void _upload_image_region_from_active_read_framebuffer() const noexcept
        requires mt::is_mutable
    {
        // gl::glCopyTextureSubImage*
    }

};




template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_GenerateMipmaps {
    JOSH3D_TEXTURE_MIXIN_HEADER

    // Wraps `glGenerateTextureMipmap`.
    void generate_mipmaps() const noexcept
        requires mt::is_mutable && tt::has_lod
    {
        gl::glGenerateTextureMipmap(self_id());
    }

};



template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations_AttachBuffer {
    JOSH3D_TEXTURE_MIXIN_HEADER

    template<of_kind<GLKind::Buffer> BufferT>
        // Are we taking ownership over the storage?
        // A mutable texture can always read/write to the buffer using server-side commands.
        requires mutability_traits<BufferT>::is_mutable
    void attach_buffer(
        const BufferT& buffer, BufferTextureInternalFormat internal_format) const noexcept
            requires mt::is_mutable
    {
        gl::glTextureBuffer(self_id(), enum_cast<GLenum>(internal_format), buffer.id());
    }

    void _attach_buffer_range() const noexcept {
        // gl::glTextureBufferRange
    }

};








template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface_ImageOperations
    : conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer && !texture_target_traits<TargetV>::is_multisample,
        TextureDSAInterface_ImageOperations_Upload<CRTP, TargetV>
    >
    , conditional_mixin_t<
        texture_target_traits<TargetV>::supports_compressed_internal_format,
        TextureDSAInterface_ImageOperations_UploadCompressed<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer && !texture_target_traits<TargetV>::is_multisample,
        TextureDSAInterface_ImageOperations_UploadFromReadFramebuffer<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer && !texture_target_traits<TargetV>::is_multisample,
        TextureDSAInterface_ImageOperations_Download<CRTP, TargetV>
    >
    , conditional_mixin_t<
        texture_target_traits<TargetV>::supports_compressed_internal_format,
        TextureDSAInterface_ImageOperations_DownloadCompressed<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer,
        TextureDSAInterface_ImageOperations_Copy<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer,
        TextureDSAInterface_ImageOperations_Fill<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer,
        TextureDSAInterface_ImageOperations_Clear<CRTP, TargetV>
    >
    // Apparently, invalidating Buffer Textures is fine.
    , TextureDSAInterface_ImageOperations_Invalidate<CRTP, TargetV>
    , conditional_mixin_t<
        texture_target_traits<TargetV>::has_lod,
        TextureDSAInterface_ImageOperations_GenerateMipmaps<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV == TextureTarget::TextureBuffer,
        TextureDSAInterface_ImageOperations_AttachBuffer<CRTP, TargetV>
    >
{};












template<typename CRTP, TextureTarget TargetV>
struct TextureDSAInterface
    : TextureDSAInterface_TextureParameters<CRTP, TargetV>
    , TextureDSAInterface_Queries<CRTP, TargetV>
    , TextureDSAInterface_Bind<CRTP, TargetV>
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer && !texture_target_traits<TargetV>::is_multisample,
        TextureDSAInterface_SamplerParameters<CRTP, TargetV>
    >
    , conditional_mixin_t<
        TargetV != TextureTarget::TextureBuffer,
        TextureDSAInterface_AllocateStorage<CRTP, TargetV>
    >
    , TextureDSAInterface_ImageOperations<CRTP, TargetV>
{};



#undef JOSH3D_TEXTURE_MIXIN_HEADER

} // namespace detail





namespace detail {

using josh::detail::RawGLHandle;

} // namespace detail




#define JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Name)                                                             \
    template<mutability_tag MutT = GLMutable>                                                                 \
    class Raw##Name                                                                                           \
        : public detail::RawGLHandle<MutT>                                                                    \
        , public detail::TextureDSAInterface<Raw##Name<MutT>, TextureTarget::Name>                            \
    {                                                                                                         \
    public:                                                                                                   \
        static constexpr GLKind        kind_type   = GLKind::Texture;                                         \
        static constexpr TextureTarget target_type = TextureTarget::Name;                                     \
        JOSH3D_MAGIC_CONSTRUCTORS_2(Raw##Name, mutability_traits<Raw##Name<MutT>>, detail::RawGLHandle<MutT>) \
    };                                                                                                        \
    static_assert(sizeof(Raw##Name<GLMutable>) == sizeof(Raw##Name<GLConst>));

JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Texture1D)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Texture1DArray)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Texture2D)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Texture2DArray)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Texture2DMS)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Texture2DMSArray)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Texture3D)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(Cubemap)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(CubemapArray)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(TextureRectangle)
JOSH3D_GENERATE_DSA_TEXTURE_CLASSES(TextureBuffer)

#undef JOSH3D_GENERATE_DSA_TEXTURE_CLASSES



} // namespace dsa
} // namespace josh
