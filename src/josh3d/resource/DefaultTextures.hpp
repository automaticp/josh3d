#pragma once
#include "GLObjects.hpp"


namespace josh {
namespace detail {

void init_default_textures();
void clear_default_textures();

} // namespace detail

namespace globals {

auto default_diffuse_texture()  noexcept -> RawTexture2D<GLConst>;
auto default_specular_texture() noexcept -> RawTexture2D<GLConst>;
auto default_normal_texture()   noexcept -> RawTexture2D<GLConst>;

auto share_default_diffuse_texture()  noexcept -> SharedConstTexture2D;
auto share_default_specular_texture() noexcept -> SharedConstTexture2D;
auto share_default_normal_texture()   noexcept -> SharedConstTexture2D;

} // namespace globals
} // namespace josh
