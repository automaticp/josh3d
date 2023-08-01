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
    Shared<TextureData> tex_data{ upstream_.load(file) };

    auto new_handle = std::make_shared<Texture2D>();

    gl::GLenum internal_format;
    switch (context.type) {
        case TextureType::diffuse:  internal_format = gl::GL_SRGB_ALPHA; break;
        case TextureType::specular: internal_format = gl::GL_RGBA; break;
        case TextureType::normal:   internal_format = gl::GL_RGBA; break;
        default: internal_format = gl::GL_RGBA;
    }

    new_handle->bind().attach_data(*tex_data, internal_format);

    return new_handle;
}







}
