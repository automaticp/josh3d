#pragma once
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "ImageData.hpp"
#include "TextureHelpers.hpp"
#include "Filesystem.hpp"
#include "GLObjectPool.hpp"
#include "DataPool.hpp"
#include <glbinding/gl/enum.h>



namespace josh {


enum class TextureType {
    default_,
    diffuse,
    specular,
    normal
    // Extend later
};

struct TextureHandleLoadContext {
    TextureType type{ TextureType::default_ };
};



using TextureHandlePool =
    GLObjectPool<RawTexture2D<>, DataPool<TextureData>, TextureHandleLoadContext>;


template<>
inline auto TextureHandlePool::load_data_from(
    const File&                     file,
    const TextureHandleLoadContext& context)
        -> SharedTexture2D
{

    Shared<TextureData> tex_data{ upstream_.load(file) };

    InternalFormat internal_format = [&] {
        switch (context.type) {
            using enum TextureType;
            case diffuse:  return InternalFormat::SRGBA8;
            case specular:
            case normal:
            default:       return InternalFormat::RGBA8;
        }
    }();

    SharedTexture2D new_handle =
        create_material_texture_from_data(*tex_data, internal_format);

    return new_handle;
}


namespace globals {
inline TextureHandlePool texture_handle_pool{ texture_data_pool };
} // namespace globals


} // namespace josh
