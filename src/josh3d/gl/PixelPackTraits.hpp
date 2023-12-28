#pragma once
#include "GLScalars.hpp"
#include "Pixels.hpp"
#include <glbinding/gl/enum.h>


namespace josh {


namespace detail {
template<typename ChanT> struct gl_channel_type;
template<> struct gl_channel_type<ubyte_t> { static constexpr GLenum value = gl::GL_UNSIGNED_BYTE; };
template<> struct gl_channel_type<float>   { static constexpr GLenum value = gl::GL_FLOAT;         };

template<typename PixelT> struct gl_pixel_format;
template<> struct gl_pixel_format<pixel::RED>  { static constexpr GLenum value = gl::GL_RED;  };
template<> struct gl_pixel_format<pixel::RG>   { static constexpr GLenum value = gl::GL_RG;   };
template<> struct gl_pixel_format<pixel::RGB>  { static constexpr GLenum value = gl::GL_RGB;  };
template<> struct gl_pixel_format<pixel::RGBA> { static constexpr GLenum value = gl::GL_RGBA; };

template<> struct gl_pixel_format<pixel::REDF>  { static constexpr GLenum value = gl::GL_RED;  };
template<> struct gl_pixel_format<pixel::RGF>   { static constexpr GLenum value = gl::GL_RG;   };
template<> struct gl_pixel_format<pixel::RGBF>  { static constexpr GLenum value = gl::GL_RGB;  };
template<> struct gl_pixel_format<pixel::RGBAF> { static constexpr GLenum value = gl::GL_RGBA; };
} // namespace detail


template<typename PixelT>
struct pixel_pack_traits {
private:
    using tr = pixel_traits<PixelT>;
public:
    static constexpr GLenum format = detail::gl_pixel_format<PixelT>::value;
    static constexpr GLenum type   = detail::gl_channel_type<typename tr::channel_type>::value;
};


} // namespace josh
