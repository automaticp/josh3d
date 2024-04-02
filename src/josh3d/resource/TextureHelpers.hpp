#pragma once
#include "GLObjectHelpers.hpp"
#include "GLScalars.hpp"
#include "GLTextures.hpp"
#include "GLObjects.hpp"
#include "ImageData.hpp"
#include "Filesystem.hpp"
#include "MallocSupport.hpp"
#include "Pixels.hpp"
#include "RuntimeError.hpp"
#include "Size.hpp"
#include "PixelPackTraits.hpp"
#include "CubemapData.hpp"
#include <glbinding/gl/enum.h>
#include <string>


namespace josh {


namespace error {

class ImageReadingError final : public RuntimeError {
public:
    static constexpr auto prefix = "Cannot Read Image: ";
    Path path;
    ImageReadingError(Path path, std::string reason)
        : RuntimeError(prefix,
            path.native() + "; Reason: " + std::move(reason))
        , path{ std::move(path) }
    {}
};

} // namespace error




template<typename PixelT>
[[nodiscard]] ImageData<PixelT>   load_image_from_file(const File& file,
    bool flip_vertically = true);

template<typename PixelT>
[[nodiscard]] CubemapData<PixelT> load_cubemap_from_files(
    const File& posx, const File& negx,
    const File& posy, const File& negy,
    const File& posz, const File& negz,
    bool flip_vertically = true);

template<typename PixelT>
[[nodiscard]] CubemapData<PixelT> load_cubemap_from_json(const File& json_file,
    bool flip_vertically = true);



// A "material" texture will have it's mipmaps generated.


template<typename PixelT>
[[nodiscard]] auto create_material_texture_from_data(
    const ImageData<PixelT>& data,
    InternalFormat           internal_format)
        -> dsa::UniqueTexture2D;

template<typename PixelT>
[[nodiscard]] auto create_material_cubemap_from_data(
    const CubemapData<PixelT>& data,
    InternalFormat             internal_format)
        -> dsa::UniqueCubemap;

template<typename PixelT>
[[nodiscard]] auto create_skybox_from_cubemap_data(
    const CubemapData<PixelT>& data,
    InternalFormat             internal_format)
        -> dsa::UniqueCubemap;





namespace detail {


template<typename ChanT>
struct ImageStorage {
    unique_malloc_ptr<ChanT[]> data;
    Size2S size;
};

template<typename ChanT>
ImageStorage<ChanT>   load_image_from_file_impl(
    const File& file, size_t n_channels, bool vflip);

extern template
ImageStorage<ubyte_t> load_image_from_file_impl<ubyte_t>(
    const File& file, size_t n_channels, bool vflip);

extern template
ImageStorage<float>   load_image_from_file_impl<float>(
    const File& file, size_t n_channels, bool vflip);


std::array<File, 6> parse_cubemap_json_for_files(const File& json_file);


} // namespace detail


template<typename PixelT>
[[nodiscard]] ImageData<PixelT> load_image_from_file(const class File& file, bool flip_vertically) {
    using tr = pixel_traits<PixelT>;

    detail::ImageStorage<typename tr::channel_type> im =
        detail::load_image_from_file_impl<typename tr::channel_type>(file, tr::n_channels, flip_vertically);

    return ImageData<PixelT>::from_channel_data(
        std::move(im.data), im.size
    );
}


template<typename PixelT>
[[nodiscard]] CubemapData<PixelT> load_cubemap_from_files(
    const File& posx, const File& negx,
    const File& posy, const File& negy,
    const File& posz, const File& negz,
    bool flip_vertically)
{
    return CubemapData<PixelT> {
        load_image_from_file<PixelT>(posx, flip_vertically),
        load_image_from_file<PixelT>(negx, flip_vertically),
        load_image_from_file<PixelT>(posy, flip_vertically),
        load_image_from_file<PixelT>(negy, flip_vertically),
        load_image_from_file<PixelT>(posz, flip_vertically),
        load_image_from_file<PixelT>(negz, flip_vertically)
    };
}


template<typename PixelT>
[[nodiscard]] CubemapData<PixelT> load_cubemap_from_json(
    const File& json_file, bool flip_vertically)
{
    std::array<File, 6> files = detail::parse_cubemap_json_for_files(json_file);
    return load_cubemap_from_files<PixelT>(
        files[0], files[1],
        files[2], files[3],
        files[4], files[5],
        flip_vertically
    );
}




template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_material_texture_from_data(
    const ImageData<PixelT>& data,
    InternalFormat           internal_format)
        -> dsa::UniqueTexture2D
{
    const Size2I resolution{ data.size() };

    dsa::UniqueTexture2D texture;
    texture->allocate_storage(resolution, internal_format, dsa::max_num_levels(resolution));
    texture->upload_image_region(Region2I{ {}, resolution }, data.data(), MipLevel{ 0 });
    texture->generate_mipmaps();
    texture->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    return texture;
}


template<specifies_pixel_pack_traits PixelT>
[[nodiscard]] auto create_material_cubemap_from_data(
    const CubemapData<PixelT>& data,
    InternalFormat             internal_format)
        -> dsa::UniqueCubemap
{
    const Size2I resolution{ data.sides()[0].size() };

    dsa::UniqueCubemap cubemap;
    cubemap->allocate_storage(resolution, internal_format, dsa::max_num_levels(resolution));

    for (GLint face_id{ 0 }; face_id < 6; ++face_id) {
        const auto face_data = data.sides()[face_id].data();
        cubemap->upload_image_region(Region3I{ { 0, 0, face_id }, { resolution, 1 } }, face_data, MipLevel{ 0 });
    }

    cubemap->generate_mipmaps();
    cubemap->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    cubemap->set_sampler_wrap_all(Wrap::ClampToEdge);

    return cubemap;
}


template<typename PixelT>
[[nodiscard]] auto create_skybox_from_cubemap_data(
    const CubemapData<PixelT>& data,
    InternalFormat             internal_format)
        -> dsa::UniqueCubemap
{
    const Size2I resolution{ data.sides()[0].size() };

    dsa::UniqueCubemap cubemap;
    cubemap->allocate_storage(resolution, internal_format, dsa::max_num_levels(resolution));

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




} // namespace josh
