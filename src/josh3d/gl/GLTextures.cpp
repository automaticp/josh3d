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
        TexSpec{ internal_format },
        TexPackSpec{ format, GL_UNSIGNED_BYTE },
        data.data()
    );
}




void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format)
{
    for (GLint face_id{ 0 }; face_id < data.data().size(); ++face_id) {
        const auto& face = data.data()[face_id];
        GLenum format = get_default_format(face.n_channels());
        cube.specify_face_image(
            face_id,
            Size2I{ face.image_size() },
            TexSpec{ internal_format },
            TexPackSpec{ format, GL_UNSIGNED_BYTE },
            face.data()
        );
    }
}


void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format, GLenum format)
{
    for (GLint face_id{ 0 }; face_id < data.data().size(); ++face_id) {
        const auto& face = data.data()[face_id];
        cube.specify_face_image(
            face_id,
            Size2I{ face.image_size() },
            TexSpec{ internal_format },
            TexPackSpec{ format, GL_UNSIGNED_BYTE },
            face.data()
        );
    }
}



} // namespace josh
