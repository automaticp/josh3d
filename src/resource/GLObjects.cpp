#include <glbinding/gl/gl.h>
#include "GLObjects.hpp"
#include "TextureData.hpp"
#include "Mesh.hpp"




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

    glTexImage2D(
        GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format),
        tex_data.width(), tex_data.height(), 0,
        format, GL_UNSIGNED_BYTE, tex_data.data()
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    return *this;
}
