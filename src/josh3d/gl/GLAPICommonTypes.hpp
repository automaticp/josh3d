#pragma once
#include "GLAPI.hpp"
#include "GLScalars.hpp"
#include "detail/StrongScalar.hpp"
#include "EnumUtils.hpp"


namespace josh {


enum class BufferMask : GLuint {
    ColorBit   = GLuint(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT),
    DepthBit   = GLuint(gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT),
    StencilBit = GLuint(gl::ClearBufferMask::GL_STENCIL_BUFFER_BIT),
};

inline BufferMask operator|(const BufferMask& lhs, const BufferMask& rhs) noexcept {
    return BufferMask{ to_underlying(lhs) | to_underlying(rhs) };
}





JOSH3D_DEFINE_STRONG_SCALAR(OffsetBytes, GLsizeiptr)
JOSH3D_DEFINE_STRONG_SCALAR(OffsetElems, GLsizeiptr)
JOSH3D_DEFINE_STRONG_SCALAR(NumElems,    GLsizeiptr)






enum class CompareOp : GLuint {
    LEqual   = GLuint(gl::GL_LEQUAL),
    GEqual   = GLuint(gl::GL_GEQUAL),
    Less     = GLuint(gl::GL_LESS),
    Greater  = GLuint(gl::GL_GREATER),
    Equal    = GLuint(gl::GL_EQUAL),
    NotEqual = GLuint(gl::GL_NOTEQUAL),
    Always   = GLuint(gl::GL_ALWAYS),
    Never    = GLuint(gl::GL_NEVER),
};




enum class MinFilter : GLuint {
    Nearest              = GLuint(gl::GL_NEAREST),
    Linear               = GLuint(gl::GL_LINEAR),
    NearestMipmapNearest = GLuint(gl::GL_NEAREST_MIPMAP_NEAREST),
    NearestMipmapLinear  = GLuint(gl::GL_NEAREST_MIPMAP_LINEAR),
    LinearMipmapNearest  = GLuint(gl::GL_LINEAR_MIPMAP_NEAREST),
    LinearMipmapLinear   = GLuint(gl::GL_LINEAR_MIPMAP_LINEAR),
};

enum class MinFilterNoLOD : GLuint {
    Nearest = GLuint(gl::GL_NEAREST),
    Linear  = GLuint(gl::GL_LINEAR),
};

enum class MagFilter : GLuint {
    Nearest = GLuint(gl::GL_NEAREST),
    Linear  = GLuint(gl::GL_LINEAR),
};

enum class Wrap : GLuint {
    Repeat                = GLuint(gl::GL_REPEAT),
    MirroredRepeat        = GLuint(gl::GL_MIRRORED_REPEAT),
    ClampToEdge           = GLuint(gl::GL_CLAMP_TO_EDGE),
    MirrorThenClampToEdge = GLuint(gl::GL_MIRROR_CLAMP_TO_EDGE),
    ClampToBorder         = GLuint(gl::GL_CLAMP_TO_BORDER),
};


// These types are used in border color and clear color API.
// They exist mostly because we want to return a single value from get_*border_color* calls.
// They are not recommended as a pixel format in pixel transfer operations.
struct RGBAUNorm {
    GLfloat r{}, g{}, b{}, a{};
};

struct RGBASNorm {
    GLint r{}, g{}, b{}, a{};
};

struct RGBAF {
    GLfloat r{}, g{}, b{}, a{};
};

struct RGBAI {
    GLint r{}, g{}, b{}, a{};
};

struct RGBAUI {
    GLuint r{}, g{}, b{}, a{};
};





} // namespace josh
