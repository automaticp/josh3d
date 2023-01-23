#include "GLObjects.hpp"
#include <glbinding/gl/gl.h>
#include "CubemapData.hpp"
#include "TextureData.hpp"



namespace learn {

namespace leaksgl {

using namespace gl;


BoundTextureHandle& BoundTextureHandle::attach_data(const TextureData& tex_data,
    GLenum internal_format, GLenum format) {

    if (format == GL_NONE) {
        switch (tex_data.n_channels()) {
            case 1ull: format = GL_RED; break;
            case 2ull: format = GL_RG; break;
            case 3ull: format = GL_RGB; break;
            case 4ull: format = GL_RGBA; break;
            default: format = GL_RED;
        }
    }


    specify_image(
        tex_data.width(), tex_data.height(),
        internal_format, format,
        GL_UNSIGNED_BYTE, tex_data.data()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    return *this;
}


BoundCubemap& BoundCubemap::attach_data(const CubemapData &cubemap_data,
    GLenum internal_format, GLenum format)
{

    auto get_default_format = [](size_t n_channels) {
        switch (n_channels) {
            case 1ull: return GL_RED;
            case 2ull: return GL_RG;
            case 3ull: return GL_RGB;
            case 4ull: return GL_RGBA;
            default:   return GL_RED;
        }
    };

    for (size_t i{ 0 }; i < cubemap_data.data().size(); ++i) {

        const auto& tex = cubemap_data.data()[i];

        GLenum current_format{ format };
        if (format == GL_NONE) {
            current_format = get_default_format(tex.n_channels());
        }

        specify_image(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            tex.width(), tex.height(),
            internal_format, current_format,
            GL_UNSIGNED_BYTE, tex.data()
        );
    }

    return *this;
}



bool ActiveShaderProgram::validate() {
    return ShaderProgram::validate(parent_id_);
}


} // namespace leaksgl

} // namespace learn
