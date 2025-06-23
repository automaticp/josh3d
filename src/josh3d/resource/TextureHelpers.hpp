#pragma once
#include "CategoryCasts.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjectHelpers.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLScalars.hpp"
#include "GLTextures.hpp"
#include "GLObjects.hpp"
#include "PixelData.hpp"
#include "ImageData.hpp"
#include "Filesystem.hpp"
#include "MallocSupport.hpp"
#include "Pixels.hpp"
#include "Errors.hpp"
#include "Region.hpp"
#include "PixelPackTraits.hpp"
#include "CubemapData.hpp"
#include <glbinding/gl/enum.h>


namespace josh {

JOSH3D_DERIVE_EXCEPTION_EX(ImageReadingError, RuntimeError, { Path path; });


template<typename ChannelT>
[[nodiscard]] auto load_image_data_from_file(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        flip_vertically = true)
        -> ImageData<ChannelT>;

template<typename PixelT>
[[nodiscard]] auto load_pixel_data_from_file(
    const File& file,
    bool        flip_vertically = true)
        -> PixelData<PixelT>;

template<typename PixelT>
[[nodiscard]] auto load_cubemap_pixel_data_from_files(
    const File& posx, const File& negx,
    const File& posy, const File& negy,
    const File& posz, const File& negz,
    bool flip_vertically = true)
        -> CubemapPixelData<PixelT>;

template<typename PixelT>
[[nodiscard]] auto load_cubemap_pixel_data_from_json(
    const File& json_file,
    bool        flip_vertically = true)
        -> CubemapPixelData<PixelT>;

auto parse_cubemap_json_for_files(
    const File& json_file)
        -> std::array<File, 6>;



// A "material" texture will have it's mipmaps generated.


template<typename ChannelT>
[[nodiscard]] auto create_material_texture_from_image_data(
    const ImageData<ChannelT>& data,
    PixelDataFormat            format,
    PixelDataType              type,
    InternalFormat             iformat)
        -> UniqueTexture2D;

template<typename ChannelT>
[[nodiscard]] auto create_material_cubemap_from_image_data(
    const CubemapImageData<ChannelT>& data,
    PixelDataFormat                   format,
    PixelDataType                     type,
    InternalFormat                    internal_format)
        -> UniqueCubemap;

template<typename ChannelT>
[[nodiscard]] auto create_skybox_from_cubemap_image_data(
    const CubemapImageData<ChannelT>& data,
    PixelDataFormat                   format,
    PixelDataType                     type,
    InternalFormat                    internal_format)
        -> UniqueCubemap;

template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_material_texture_from_pixel_data(
    const PixelData<PixelT>& data,
    InternalFormat           internal_format)
        -> UniqueTexture2D;

template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_material_cubemap_from_pixel_data(
    const CubemapPixelData<PixelT>& data,
    InternalFormat                  internal_format)
        -> UniqueCubemap;

template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_skybox_from_cubemap_pixel_data(
    const CubemapPixelData<PixelT>& data,
    InternalFormat                  internal_format)
        -> UniqueCubemap;





namespace detail {


template<typename ChanT>
struct UntypedImageLoadResult {
    unique_malloc_ptr<ChanT[]> data;
    Size2S                     resolution;
    size_t                     num_channels;
    size_t                     num_channels_in_file;
};

template<typename ChanT>
auto load_image_from_file_impl(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        vflip)
        -> UntypedImageLoadResult<ChanT>;

extern template
auto load_image_from_file_impl<chan::UByte>(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        vflip)
        -> UntypedImageLoadResult<chan::UByte>;

extern template
auto load_image_from_file_impl<chan::Float>(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        vflip)
        -> UntypedImageLoadResult<chan::Float>;


} // namespace detail


template<typename ChannelT>
[[nodiscard]] auto load_image_data_from_file(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        flip_vertically)
        -> ImageData<ChannelT>
{
    detail::UntypedImageLoadResult<ChannelT> im =
        detail::load_image_from_file_impl<ChannelT>(file, min_channels, max_channels, flip_vertically);

    return ImageData<ChannelT>::take_ownership(MOVE(im.data), im.resolution, im.num_channels);
}


template<typename PixelT>
[[nodiscard]] auto load_pixel_data_from_file(
    const class File& file,
    bool              flip_vertically)
        -> PixelData<PixelT>
{
    using tr = pixel_traits<PixelT>;

    detail::UntypedImageLoadResult<typename tr::channel_type> im =
        detail::load_image_from_file_impl<typename tr::channel_type>(file, tr::n_channels, tr::n_channels, flip_vertically);

    return PixelData<PixelT>::from_channel_data(
        std::move(im.data), im.resolution
    );
}


template<typename PixelT>
[[nodiscard]] auto load_cubemap_pixel_data_from_files(
    const File& posx, const File& negx,
    const File& posy, const File& negy,
    const File& posz, const File& negz,
    bool flip_vertically)
        -> CubemapPixelData<PixelT>
{
    return CubemapPixelData<PixelT> {
        load_pixel_data_from_file<PixelT>(posx, flip_vertically),
        load_pixel_data_from_file<PixelT>(negx, flip_vertically),
        load_pixel_data_from_file<PixelT>(posy, flip_vertically),
        load_pixel_data_from_file<PixelT>(negy, flip_vertically),
        load_pixel_data_from_file<PixelT>(posz, flip_vertically),
        load_pixel_data_from_file<PixelT>(negz, flip_vertically)
    };
}


template<typename PixelT>
[[nodiscard]] auto load_cubemap_pixel_data_from_json(
    const File& json_file,
    bool        flip_vertically)
        -> CubemapPixelData<PixelT>
{
    std::array<File, 6> files = parse_cubemap_json_for_files(json_file);
    return load_cubemap_pixel_data_from_files<PixelT>(
        files[0], files[1],
        files[2], files[3],
        files[4], files[5],
        flip_vertically
    );
}



template<typename ChannelT>
[[nodiscard]] auto create_material_texture_from_image_data(
    const ImageData<ChannelT>&  data,
    PixelDataFormat             format,
    PixelDataType               type,
    InternalFormat              iformat)
        -> UniqueTexture2D
{
    const Size2I resolution{ data.resolution() };

    UniqueTexture2D texture;
    texture->allocate_storage(resolution, iformat, max_num_levels(resolution));
    texture->upload_image_region({ {}, resolution }, format, type, data.data(), MipLevel{ 0 });
    texture->generate_mipmaps();
    texture->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    return texture;
}



template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_material_texture_from_pixel_data(
    const PixelData<PixelT>& data,
    InternalFormat           internal_format)
        -> UniqueTexture2D
{
    const Size2I resolution{ data.resolution() };

    UniqueTexture2D texture;
    texture->allocate_storage(resolution, internal_format, max_num_levels(resolution));
    texture->upload_image_region(Region2I{ {}, resolution }, data.data(), MipLevel{ 0 });
    texture->generate_mipmaps();
    texture->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    return texture;
}


template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_material_cubemap_from_pixel_data(
    const CubemapPixelData<PixelT>& data,
    InternalFormat                  internal_format)
        -> UniqueCubemap
{
    const Size2I resolution{ data.sides()[0].resolution() };

    UniqueCubemap cubemap;
    cubemap->allocate_storage(resolution, internal_format, max_num_levels(resolution));

    for (GLint face_id{ 0 }; face_id < 6; ++face_id) {
        const auto face_data = data.sides()[face_id].data();
        cubemap->upload_image_region(Region3I{ { 0, 0, face_id }, { resolution, 1 } }, face_data, MipLevel{ 0 });
    }

    cubemap->generate_mipmaps();
    cubemap->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    cubemap->set_sampler_wrap_all(Wrap::ClampToEdge);

    return cubemap;
}


template<typename ChannelT>
[[nodiscard]] auto create_material_cubemap_from_image_data(
    const CubemapImageData<ChannelT>& data,
    PixelDataFormat                   format,
    PixelDataType                     type,
    InternalFormat                    internal_format)
        -> UniqueCubemap
{
    const Size2I resolution{ data.sides()[0].resolution() };

    UniqueCubemap cubemap;
    cubemap->allocate_storage(resolution, internal_format, max_num_levels(resolution));

    for (GLint face_id{ 0 }; face_id < 6; ++face_id) {
        const auto face_data = data.sides()[face_id].data();
        const Region3I region{ { 0, 0, face_id }, { resolution, 1 } };
        cubemap->upload_image_region(region, format, type, face_data, MipLevel{ 0 });
    }

    cubemap->generate_mipmaps();
    cubemap->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    cubemap->set_sampler_wrap_all(Wrap::ClampToEdge);

    return cubemap;
}


template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_skybox_from_cubemap_pixel_data(
    const CubemapPixelData<PixelT>& data,
    InternalFormat                  internal_format)
        -> UniqueCubemap
{
    const Size2I resolution{ data.sides()[0].resolution() };

    UniqueCubemap cubemap;
    cubemap->allocate_storage(resolution, internal_format, max_num_levels(resolution));

    for (GLint face_id{ 0 }; face_id < 6; ++face_id) {
        const auto face_data = data.sides()[face_id].data();
        // We swap +Y and -Y faces when attaching them to the cubemap.
        // Then, inverting the X and Y coordinates in the shader
        // produces "reasonable" results.
        //
        // TODO: The skybox saga isn't over yet, but if this is a sufficent
        // solution, then remove this TODO and resume enjoying life.
        auto target_face_id = face_id;
        switch (target_face_id) {
            case 2: target_face_id = 3; break;
            case 3: target_face_id = 2; break;
            default: break;
        }
        cubemap->upload_image_region(Region3I{ { 0, 0, target_face_id }, { resolution, 1 } }, face_data, MipLevel{ 0 });
    }

    cubemap->generate_mipmaps();
    cubemap->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    cubemap->set_sampler_wrap_all(Wrap::ClampToEdge);

    return cubemap;
}


template<typename ChannelT>
[[nodiscard]] auto create_skybox_from_cubemap_image_data(
    const CubemapImageData<ChannelT>& data,
    PixelDataFormat                   format,
    PixelDataType                     type,
    InternalFormat                    internal_format)
        -> UniqueCubemap
{
    const Size2I resolution{ data.sides()[0].resolution() };

    UniqueCubemap cubemap;
    cubemap->allocate_storage(resolution, internal_format, max_num_levels(resolution));

    for (GLint face_id{ 0 }; face_id < 6; ++face_id) {
        const auto face_data = data.sides()[face_id].data();
        auto target_face_id = face_id;
        switch (target_face_id) {
            case 2: target_face_id = 3; break;
            case 3: target_face_id = 2; break;
            default: break;
        }
        const Region3I region{ { 0, 0, target_face_id }, { resolution, 1 } };
        cubemap->upload_image_region(region, format, type, face_data, MipLevel{ 0 });
    }

    cubemap->generate_mipmaps();
    cubemap->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    cubemap->set_sampler_wrap_all(Wrap::ClampToEdge);

    return cubemap;
}


} // namespace josh
