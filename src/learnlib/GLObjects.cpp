#include "GLObjects.hpp"
#include <glbinding/gl/gl.h>
#include "TextureData.hpp"



namespace learn {

namespace leaksgl {

using namespace gl;


BoundTextureHandle& BoundTextureHandle::attach_data(const TextureData& tex_data,
    GLint internal_format, GLenum format) {

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

} // namespace leaksgl

} // namespace learn
