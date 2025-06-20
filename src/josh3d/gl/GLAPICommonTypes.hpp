#pragma once
#include "GLAPI.hpp"
#include "GLScalars.hpp"
#include "detail/StrongScalar.hpp"
#include "EnumUtils.hpp"


/*
Common or shared vocabulary of GLAPI.
Types that are used in multiple places will likely end up here.
*/
namespace josh {

/*
General.
*/
struct RangeF
{
    GLfloat min = {};
    GLfloat max = {};
};

struct RGBAUNorm
{
    GLfloat r{}, g{}, b{}, a{};
};

struct RGBASNorm
{
    GLint r{}, g{}, b{}, a{};
};

struct RGBAF
{
    GLfloat r{}, g{}, b{}, a{};
};

struct RGBAI
{
    GLint r{}, g{}, b{}, a{};
};

struct RGBAUI
{
    GLuint r{}, g{}, b{}, a{};
};

/*
Draw and dispatch commands.
*/
struct DrawArraysIndirectCommand
{
    GLuint vertex_count;
    GLuint instance_count;
    GLuint vertex_offset;
    GLuint base_instance;
};

struct DrawElementsIndirectCommand
{
    GLuint element_count;
    GLuint instance_count;
    GLuint element_offset;
    GLint  base_vertex;
    GLuint base_instance;
};

struct DispatchIndirectCommand
{
    GLuint num_groups_x;
    GLuint num_groups_y;
    GLuint num_groups_z;
};

/*
Per-face parameters.
*/
enum class Face : GLuint
{
    Front = GLuint(gl::GL_FRONT),
    Back  = GLuint(gl::GL_BACK),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Face, Front, Back);

/*
Stencil test mask. TODO: Name should be more specific.
*/
JOSH3D_DEFINE_STRONG_SCALAR(Mask, GLuint);

/*
Buffer vocabulary.
*/
JOSH3D_DEFINE_STRONG_SCALAR(OffsetBytes, GLsizeiptr);
JOSH3D_DEFINE_STRONG_SCALAR(OffsetElems, GLsizeiptr);
JOSH3D_DEFINE_STRONG_SCALAR(NumElems,    GLsizeiptr);

struct ElemRange
{
    OffsetElems offset;
    NumElems    count;
};

/*
TODO: Is this used anywhere? Is this acceptable vocabulary?
*/
#if 0
JOSH3D_DEFINE_STRONG_SCALAR(NumBytes,    GLsizeiptr);
struct ByteRange
{
    OffsetBytes byte_offset;
    NumBytes    byte_count;
}
#endif

/*
Texture and sampler vocabulary.
*/
JOSH3D_DEFINE_STRONG_SCALAR(Layer, GLint);
JOSH3D_DEFINE_STRONG_SCALAR(MipLevel,  GLint);
JOSH3D_DEFINE_STRONG_SCALAR(NumLevels, GLsizei);

/*
TODO: Move this quote somewhere else...

"[8.8] samples represents a request for a desired minimum number of samples.
Since different implementations may support different sample counts for multi-
sampled textures, the actual number of samples allocated for the texture image is
implementation-dependent. However, the resulting value for TEXTURE_SAMPLES
is guaranteed to be greater than or equal to samples and no more than the next
larger sample count supported by the implementation."
*/
JOSH3D_DEFINE_STRONG_SCALAR(NumSamples, GLsizei);

enum class CompareOp : GLuint
{
    LEqual   = GLuint(gl::GL_LEQUAL),
    GEqual   = GLuint(gl::GL_GEQUAL),
    Less     = GLuint(gl::GL_LESS),
    Greater  = GLuint(gl::GL_GREATER),
    Equal    = GLuint(gl::GL_EQUAL),
    NotEqual = GLuint(gl::GL_NOTEQUAL),
    Always   = GLuint(gl::GL_ALWAYS),
    Never    = GLuint(gl::GL_NEVER),
};
JOSH3D_DEFINE_ENUM_EXTRAS(CompareOp, LEqual, GEqual, Less, Greater, Equal, NotEqual, Always, Never);

enum class MinFilter : GLuint
{
    Nearest              = GLuint(gl::GL_NEAREST),
    Linear               = GLuint(gl::GL_LINEAR),
    NearestMipmapNearest = GLuint(gl::GL_NEAREST_MIPMAP_NEAREST),
    NearestMipmapLinear  = GLuint(gl::GL_NEAREST_MIPMAP_LINEAR),
    LinearMipmapNearest  = GLuint(gl::GL_LINEAR_MIPMAP_NEAREST),
    LinearMipmapLinear   = GLuint(gl::GL_LINEAR_MIPMAP_LINEAR),
};
JOSH3D_DEFINE_ENUM_EXTRAS(MinFilter,
    Nearest,
    Linear,
    NearestMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapNearest,
    LinearMipmapLinear);

enum class MinFilterNoLOD : GLuint
{
    Nearest = GLuint(gl::GL_NEAREST),
    Linear  = GLuint(gl::GL_LINEAR),
};
JOSH3D_DEFINE_ENUM_EXTRAS(MinFilterNoLOD, Nearest, Linear);

enum class MagFilter : GLuint
{
    Nearest = GLuint(gl::GL_NEAREST),
    Linear  = GLuint(gl::GL_LINEAR),
};
JOSH3D_DEFINE_ENUM_EXTRAS(MagFilter, Nearest, Linear);

enum class Wrap : GLuint
{
    Repeat                = GLuint(gl::GL_REPEAT),
    MirroredRepeat        = GLuint(gl::GL_MIRRORED_REPEAT),
    ClampToEdge           = GLuint(gl::GL_CLAMP_TO_EDGE),
    MirrorThenClampToEdge = GLuint(gl::GL_MIRROR_CLAMP_TO_EDGE),
    ClampToBorder         = GLuint(gl::GL_CLAMP_TO_BORDER),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Wrap, Repeat, MirroredRepeat, ClampToEdge, MirrorThenClampToEdge, ClampToBorder);

enum class Swizzle : GLuint
{
    Red   = GLuint(gl::GL_RED),
    Green = GLuint(gl::GL_GREEN),
    Blue  = GLuint(gl::GL_BLUE),
    Alpha = GLuint(gl::GL_ALPHA),
    Zero  = GLuint(gl::GL_ZERO),
    One   = GLuint(gl::GL_ONE),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Swizzle, Red, Green, Blue, Alpha, Zero, One);

struct SwizzleRGBA
{
    Swizzle r = Swizzle::Red;
    Swizzle g = Swizzle::Green;
    Swizzle b = Swizzle::Blue;
    Swizzle a = Swizzle::Alpha;

    // Constructs a SwizzleRGBA with all channel slots set to `s`.
    constexpr static auto all(Swizzle s) noexcept
        -> SwizzleRGBA
    {
        return { s, s, s, s };
    }

    // Returns the number of channel slots that are not Zero or One. Up to 4.
    constexpr auto num_nonconst() const noexcept
        -> size_t
    {
        const auto nonconst = [](Swizzle s)
            -> bool
        {
            return (s != Swizzle::Zero) and (s != Swizzle::One);
        };
        return nonconst(r) + nonconst(g) + nonconst(b) + nonconst(a);
    }

    // Returns the number of unique nonconst target channels referenced by this `SwizzleRGBA`. Up to 4.
    // For example, the number of unique nonconst target channels in `SwizzleRGBA{ Red, Red, Zero, Blue }`
    // is 2, and in `SwizzleRGBA{ Red, Zero, One, Red }` is 1.
    constexpr auto num_unique_nonconst() const noexcept
        -> size_t
    {
        bool nonconst[4] = {};
        const auto mark = [&](Swizzle s)
        {
            switch (s)
            {
                case Swizzle::Red:   nonconst[0] = true; break;
                case Swizzle::Green: nonconst[1] = true; break;
                case Swizzle::Blue:  nonconst[2] = true; break;
                case Swizzle::Alpha: nonconst[3] = true; break;
                default: break;
            }
        };
        mark(r); mark(g); mark(b); mark(a);
        return nonconst[0] + nonconst[1] + nonconst[2] + nonconst[3];
    }

    constexpr auto operator==(const SwizzleRGBA&) const noexcept -> bool = default;
};


} // namespace josh
