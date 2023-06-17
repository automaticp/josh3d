#pragma once
#include <concepts>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include "AndThen.hpp"
#include "GLObjectAllocators.hpp"
#include "GLBuffers.hpp"
#include "GLShaders.hpp"


namespace learn {


/*
This file defines thin wrappers around various OpenGL objects: Buffers, Arrays, Textures, Shaders, etc.
Each GL object wrapper defines a 'Bound' dummy nested class that permits actions,
which are only applicable to bound or in-use GL objects.

Bound dummies do not perform any sanity checks for actually being bound or being used in correct context.
Their lifetimes do not end when the parent object is unbound.
Use-after-unbinding is still a programmer error.
It's recommended to use them as rvalues whenever possible; their methods support chaining.

The interface of Bound dummies serves as a guide for establising dependencies
between GL objects and correct order of calls to OpenGL API.

The common pattern for creating a Vertex Array (VAO) with a Vertex Buffer (VBO)
and an Element Buffer (EBO) attached in terms of these wrappes looks like this:

    VAO vao;
    VBO vbo;
    EBO ebo;
    auto bvao = vao.bind();
    vbo.bind().attach_data(...).associate_with(bvao, attribute_layout);
    ebo.bind(bvao).attach_data(...);
    bvao.unbind();

From the example above you can infer that the association between VAO and VBO is made
during the VBO::associate_with(...) call (glVertexAttribPointer(), in particular),
whereas the EBO is associated with a currently bound VAO when it gets bound itself.

The requirement to pass a reference to an existing VAO::Bound dummy during these calls
also implies their dependency on the currently bound Vertex Array. It would not make
sense to make these calls in absence of a bound VAO.
*/


class TextureData;
class CubemapData;

/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/

namespace leaksgl {

using namespace gl;

class VAO;
class VBO;
class EBO;
class BoundVAO;
class BoundVBO;
class BoundEBO;


// P.S. Some definitions were moved to a *.cpp file to break dependencies




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
        const void* data, GLint mipmap_level = 0) {

        glTexImage2D(
            GL_TEXTURE_2D, mipmap_level, static_cast<GLint>(internal_format),
            width, height, 0, format, type, data
        );
        return *this;
    }

    // Technically applies to Active Tex Unit and not to the Bound Texture directly
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

    BoundTextureMS& specify_image(GLsizei width, GLsizei height, GLsizei nsamples,
        GLenum internal_format = GL_RGB, GLboolean fixed_sample_locations = GL_TRUE)
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

    BoundCubemap& specify_image(GLenum target, GLsizei width, GLsizei height,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
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

    BoundCubemapArray& specify_all_images(GLsizei width, GLsizei height, GLsizei depth,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
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
