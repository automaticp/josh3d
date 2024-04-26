#include "DefaultTextures.hpp"
#include "PixelData.hpp"
#include "TextureHelpers.hpp"
#include <optional>


namespace josh {

namespace {

template<typename PixelT>
auto make_single_pixel_image_data(PixelT pixel)
    -> PixelData<PixelT>
{
    PixelData<PixelT> image{ { 1, 1 } };
    image.at({ 0, 0 }) = pixel;
    return image;
}


template<typename PixelT>
auto create_single_pixel_texture(
    PixelT pixel,
    InternalFormat iformat)
        -> UniqueTexture2D
{
    auto tex_data = make_single_pixel_image_data(pixel);
    UniqueTexture2D texture = create_material_texture_from_pixel_data(tex_data, iformat);
    return texture;
}


std::optional<UniqueTexture2D> default_diffuse_texture_;
std::optional<UniqueTexture2D> default_specular_texture_;
std::optional<UniqueTexture2D> default_normal_texture_;


} // namespace


namespace detail {

void init_default_textures() {
    default_diffuse_texture_  = create_single_pixel_texture(pixel::RGB{ 0xB0, 0xB0, 0xB0 }, InternalFormat::SRGB8);
    default_specular_texture_ = create_single_pixel_texture(pixel::Red{ 0x00 },             InternalFormat::R8);
    default_normal_texture_   = create_single_pixel_texture(pixel::RGB{ 0x7F, 0x7F, 0xFF }, InternalFormat::RGB8);
}

void clear_default_textures() {
    default_diffuse_texture_ .reset();
    default_specular_texture_.reset();
    default_normal_texture_  .reset();
}

} // namespace detail


namespace globals {


RawTexture2D<GLConst> default_diffuse_texture()  noexcept { return default_diffuse_texture_ .value(); }
RawTexture2D<GLConst> default_specular_texture() noexcept { return default_specular_texture_.value(); }
RawTexture2D<GLConst> default_normal_texture()   noexcept { return default_normal_texture_  .value(); }




} // namespace globals




} // namespace josh
