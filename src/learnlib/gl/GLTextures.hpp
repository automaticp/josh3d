#pragma once
#include "GLObjectAllocators.hpp"
#include "AndThen.hpp"
#include <glbinding/gl/gl.h>




namespace learn {


/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/


class TextureData;
class CubemapData;


namespace leaksgl {

using namespace gl;




class BoundTextureHandle : public detail::AndThen<BoundTextureHandle> {
private:
    friend class TextureHandle;
    BoundTextureHandle() = default;

public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    BoundTextureHandle& attach_data(const TextureData& tex_data,
        GLenum internal_format = GL_RGBA, GLenum format = GL_NONE);

    BoundTextureHandle& specify_image(GLsizei width, GLsizei height,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        glTexImage2D(
            GL_TEXTURE_2D, mipmap_level, static_cast<GLint>(internal_format),
            width, height, 0, format, type, data
        );
        return *this;
    }

    // Technically applies to Active Tex Unit and
    // not to the Bound Texture directly
    BoundTextureHandle& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_2D, param_name, param_value);
        return *this;
    }

    BoundTextureHandle& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_2D, param_name, param_value);
        return *this;
    }

    BoundTextureHandle& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_2D, param_name, param_value);
        return *this;
    }

    BoundTextureHandle& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_2D, param_name, param_values);
        return *this;
    }

};


class TextureHandle : public TextureAllocator {
public:
    BoundTextureHandle bind() {
        glBindTexture(GL_TEXTURE_2D, id_);
        return {};
    }

    BoundTextureHandle bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};











class BoundTextureMS : public detail::AndThen<BoundTextureMS> {
private:
    friend class TextureMS;
    BoundTextureMS() = default;

public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

    BoundTextureMS& specify_image(GLsizei width, GLsizei height,
        GLsizei nsamples, GLenum internal_format = GL_RGB,
        GLboolean fixed_sample_locations = GL_TRUE)
    {
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, nsamples, internal_format, width, height, fixed_sample_locations);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_value);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_value);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_value);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_values);
        return *this;
    }


};


class TextureMS : public TextureAllocator {
public:
    BoundTextureMS bind() {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id_);
        return {};
    }

    BoundTextureMS bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};











class BoundCubemap : public detail::AndThen<BoundCubemap> {
private:
    friend class Cubemap;
    BoundCubemap() = default;
public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    BoundCubemap& specify_image(GLenum target, GLsizei width,
        GLsizei height, GLenum internal_format, GLenum format,
        GLenum type, const void* data, GLint mipmap_level = 0)
    {
        glTexImage2D(
            target, mipmap_level, internal_format,
            width, height, 0, format, type, data
        );
        return *this;
    }

    BoundCubemap& specify_all_images(GLsizei width, GLsizei height,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        for (size_t i{ 0 }; i < 6; ++i) {
            specify_image(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                width, height,
                internal_format, format,
                type, data, mipmap_level
            );
        }
        return *this;
    }


    BoundCubemap& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, param_name, param_value);
        return *this;
    }

    BoundCubemap& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, param_name, param_value);
        return *this;
    }

    BoundCubemap& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_CUBE_MAP, param_name, param_value);
        return *this;
    }

    BoundCubemap& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_CUBE_MAP, param_name, param_values);
        return *this;
    }


    BoundCubemap& attach_data(const CubemapData& tex_data,
        GLenum internal_format = GL_RGB, GLenum format = GL_NONE);


};


class Cubemap : public TextureAllocator {
public:
    Cubemap() {
        this->bind()
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE)
            .unbind();
    }

    BoundCubemap bind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
        return {};
    }

    BoundCubemap bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};











class BoundCubemapArray : public detail::AndThen<BoundCubemapArray> {
private:
    friend class CubemapArray;
    BoundCubemapArray() = default;
public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    }

    BoundCubemapArray& specify_all_images(GLsizei width,
        GLsizei height, GLsizei depth, GLenum internal_format,
        GLenum format, GLenum type, const void* data,
        GLint mipmap_level = 0)
    {
        glTexImage3D(
            GL_TEXTURE_CUBE_MAP_ARRAY, mipmap_level, internal_format,
            width, height, 6 * depth, 0, format, type, data
        );
        return *this;
    }


    BoundCubemapArray& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_value);
        return *this;
    }

    BoundCubemapArray& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_value);
        return *this;
    }

    BoundCubemapArray& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_value);
        return *this;
    }

    BoundCubemapArray& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_values);
        return *this;
    }

};


class CubemapArray : public TextureAllocator {
public:
    BoundCubemapArray bind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, id_);
        return {};
    }

    BoundCubemapArray bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};






} // namespace leaksgl

using leaksgl::BoundTextureHandle, leaksgl::TextureHandle;
using leaksgl::BoundTextureMS, leaksgl::TextureMS;
using leaksgl::BoundCubemap, leaksgl::Cubemap;
using leaksgl::BoundCubemapArray, leaksgl::CubemapArray;

} // namespace learn
