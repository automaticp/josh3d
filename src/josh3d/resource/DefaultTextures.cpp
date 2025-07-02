#include "DefaultTextures.hpp"
#include "Common.hpp"
#include "PixelData.hpp"
#include "TextureHelpers.hpp"


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
    PixelT         pixel,
    InternalFormat iformat)
        -> UniqueTexture2D
{
    auto tex_data = make_single_pixel_image_data(pixel);
    return create_material_texture_from_pixel_data(tex_data, iformat);
}

Optional<SharedTexture2D> _default_diffuse_texture;
Optional<SharedTexture2D> _default_specular_texture;
Optional<SharedTexture2D> _default_normal_texture;

} // namespace


namespace detail {

void init_default_textures()
{
    _default_diffuse_texture  = create_single_pixel_texture(pixel::RGB{ 0xB0, 0xB0, 0xB0 }, InternalFormat::SRGB8);
    _default_specular_texture = create_single_pixel_texture(pixel::Red{ 0x00 },             InternalFormat::R8);
    _default_normal_texture   = create_single_pixel_texture(pixel::RGB{ 0x7F, 0x7F, 0xFF }, InternalFormat::RGB8);
}

void clear_default_textures()
{
    _default_diffuse_texture .reset();
    _default_specular_texture.reset();
    _default_normal_texture  .reset();
}

} // namespace detail


namespace globals {

auto default_diffuse_texture()  noexcept -> RawTexture2D<GLConst> { return _default_diffuse_texture .value(); }
auto default_specular_texture() noexcept -> RawTexture2D<GLConst> { return _default_specular_texture.value(); }
auto default_normal_texture()   noexcept -> RawTexture2D<GLConst> { return _default_normal_texture  .value(); }

auto share_default_diffuse_texture()  noexcept -> SharedConstTexture2D { return _default_diffuse_texture .value(); }
auto share_default_specular_texture() noexcept -> SharedConstTexture2D { return _default_specular_texture.value(); }
auto share_default_normal_texture()   noexcept -> SharedConstTexture2D { return _default_normal_texture  .value(); }

} // namespace globals


} // namespace josh
