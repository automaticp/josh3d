#include "GLTextures.hpp"
#include "CubemapData.hpp"
#include "TextureData.hpp"


using namespace gl;


namespace josh {


static GLenum get_default_format(size_t n_channels) noexcept {
    switch (n_channels) {
        case 1ull: return GL_RED;
        case 2ull: return GL_RG;
        case 3ull: return GL_RGB;
        case 4ull: return GL_RGBA;
        default:   return GL_RED;
    }
}


void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const TextureData& data, GLenum internal_format)
{
    GLenum format = get_default_format(data.n_channels());
    attach_data_to_texture(tex, data, internal_format, format);
}


void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const TextureData& data, GLenum internal_format, GLenum format)
{
    tex.specify_image(
        Size2I{ data.image_size() },
        GLTexSpec<GL_TEXTURE_2D>{ internal_format, format, GL_UNSIGNED_BYTE },
        data.data()
    );
}




void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format)
{
    for (size_t i{ 0 }; i < data.data().size(); ++i) {
        const auto& face = data.data()[i];
        GLenum format = get_default_format(face.n_channels());
        cube.specify_image(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            Size2I{ face.image_size() },
            { internal_format, format, GL_UNSIGNED_BYTE },
            face.data()
        );
    }
}


void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format, GLenum format)
{
    for (size_t i{ 0 }; i < data.data().size(); ++i) {
        const auto& face = data.data()[i];
        cube.specify_image(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            Size2I{ face.image_size() },
            { internal_format, format, GL_UNSIGNED_BYTE },
            face.data()
        );
    }
}



} // namespace josh
