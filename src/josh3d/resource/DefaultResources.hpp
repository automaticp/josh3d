#pragma once
#include "GLMutability.hpp"
#include "GLTextures.hpp"


namespace josh {


namespace globals {
RawTexture2D<GLConst> default_diffuse_texture()  noexcept;
RawTexture2D<GLConst> default_specular_texture() noexcept;
RawTexture2D<GLConst> default_normal_texture()   noexcept;
} // namespace globals


namespace detail {
void init_default_textures();
void reset_default_textures();
} // namespace detail


} // namespace josh
