#pragma once
#include "GLScalars.hpp"
#include "GLTextures.hpp"
#include "GLMutability.hpp"
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


template<typename PixelT>
void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const ImageData<PixelT>& data, const TexSpec& spec);

template<typename PixelT>
void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData<PixelT>& data, const TexSpec& spec);

template<typename PixelT>
void attach_data_to_cubemap_as_skybox(BoundCubemap<GLMutable>& cube,
    const CubemapData<PixelT>& data, const TexSpec& spec, GLenum filter_mode = gl::GL_LINEAR);


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
ImageData<PixelT> load_image_from_file(const class File& file, bool flip_vertically) {
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


template<typename PixelT>
void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const ImageData<PixelT>& data, const TexSpec& spec)
{
    using tr = pixel_pack_traits<PixelT>;
    tex.specify_image(
        Size2I{ data.size() }, spec, TexPackSpec{ GLenum(tr::format), GLenum(tr::type) }, data.data()
    );
}


template<typename PixelT>
void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData<PixelT>& data, const TexSpec& spec)
{
    using tr = pixel_pack_traits<PixelT>;
    for (GLint face_id{ 0 }; face_id < data.sides().size(); ++face_id) {
        const auto& face = data.sides()[face_id];
        cube.specify_face_image(
            face_id,
            Size2I{ face.size() }, spec, TexPackSpec{ GLenum(tr::format), GLenum(tr::type) }, face.data()
        );
    }
}


template<typename PixelT>
void attach_data_to_cubemap_as_skybox(BoundCubemap<GLMutable>& cube,
    const CubemapData<PixelT>& data, const TexSpec& spec, GLenum filter_mode)
{
    using tr = pixel_pack_traits<PixelT>;
    for (GLint face_id{ 0 }; face_id < data.sides().size(); ++face_id) {
        const auto& face = data.sides()[face_id];
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
        cube.specify_face_image(
            target_face_id,
            Size2I{ face.size() }, spec, TexPackSpec{ GLenum(tr::format), GLenum(tr::type) }, face.data()
        );
    }
    using enum GLenum;
    cube.set_min_mag_filters(filter_mode, filter_mode)
        .set_wrap_st(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}




} // namespace josh
