#pragma once
#include "GLObjects.hpp"
#include "TextureData.hpp"
#include "Filesystem.hpp"
#include "GLObjectPool.hpp"
#include "DataPool.hpp"
#include <glbinding/gl/enum.h>
#include <concepts>
#include <type_traits>



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
    GLObjectPool<Texture2D, DataPool<TextureData>, TextureHandleLoadContext>;


template<>
Shared<Texture2D>
inline TextureHandlePool::load_data_from(const File& file,
    const TextureHandleLoadContext& context)
{
    using enum GLenum;

    Shared<TextureData> tex_data{ upstream_.load(file) };

    auto new_handle = std::make_shared<Texture2D>();

    GLenum internal_format = [&] {
        switch (context.type) {
            using enum TextureType;
            case diffuse:  return GL_SRGB_ALPHA;
            case specular:
            case normal:
            default:       return GL_RGBA;
        }
    }();

    new_handle->bind()
        .and_then([&](BoundTexture2D<GLMutable>& tex) {
            attach_data_to_texture(tex, *tex_data, internal_format);
        })
        .generate_mipmaps()
        .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    return new_handle;
}







}
