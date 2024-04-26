#pragma once
#include "GLObjects.hpp"


namespace josh {

namespace detail {

void init_default_textures();
void clear_default_textures();

} // namespace detail

namespace globals {

RawTexture2D<GLConst> default_diffuse_texture()  noexcept;
RawTexture2D<GLConst> default_specular_texture() noexcept;
RawTexture2D<GLConst> default_normal_texture()   noexcept;

} // namespace globals

} // namespace josh
