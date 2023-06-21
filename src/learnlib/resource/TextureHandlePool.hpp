#pragma once
#include "GLObjects.hpp"
#include "TextureData.hpp"
#include "GLObjectPool.hpp"
#include <concepts>
#include <glbinding/gl/enum.h>
#include <type_traits>



namespace learn {


enum class TextureType {
    default_,
    diffuse,
    specular
    // Extend later
};

struct TextureHandleLoadContext {
    TextureType type{ TextureType::default_ };
};



using TextureHandlePool =
    GLObjectPool<Texture2D, DataPool<TextureData>, TextureHandleLoadContext>;


template<>
Shared<Texture2D>
inline TextureHandlePool::load_data_from(const std::string& path,
    const TextureHandleLoadContext& context)
{
    Shared<TextureData> tex_data{ upstream_.load(path) };

    auto new_handle = std::make_shared<Texture2D>();

    gl::GLenum internal_format;
    switch (context.type) {
        case TextureType::diffuse: internal_format = gl::GL_SRGB_ALPHA; break;
        case TextureType::specular: internal_format = gl::GL_RGBA; break;
        default: internal_format = gl::GL_RGBA;
    }

    new_handle->bind().attach_data(*tex_data, internal_format);

    return new_handle;
}







}
