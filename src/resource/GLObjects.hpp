#pragma once
#include <glbinding/gl/gl.h>
#include <array>
#include "GLObjectAllocators.hpp"
#include "TextureData.hpp"
#include "Vertex.hpp"


namespace learn {

using namespace gl;


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



class VAO;
class VBO;
class EBO;
class BoundVAO;
class BoundVBO;
class BoundEBO;

// P.S. Some definitions were moved to a *.cpp file to break dependencies


class BoundVAO {
private:
    friend class VAO;
    BoundVAO() = default;

public:
    BoundVAO& enable_array_access(GLuint attrib_index) {
        glEnableVertexAttribArray(attrib_index);
        return *this;
    }

    BoundVAO& disable_array_access(GLuint attrib_index) {
        glDisableVertexAttribArray(attrib_index);
        return *this;
    }

    BoundVAO& draw_arrays(GLenum mode, GLint first, GLsizei count) {
        glDrawArrays(mode, first, count);
        return *this;
    }

    BoundVAO& draw_elements(GLenum mode, GLsizei count, GLenum type,
                    const void* indices_buffer = nullptr) {
        glDrawElements(mode, count, type, indices_buffer);
        return *this;
    }

    template<size_t N>
    BoundVAO& set_many_attribute_params(
        const std::array<AttributeParams, N>& aparams) {

        for (const auto& ap : aparams) {
            set_attribute_params(ap);
            this->enable_array_access(ap.index);
        }
        return *this;
    }

    template<size_t N>
    BoundVAO& associate_with(BoundVBO& vbo,
                        const std::array<AttributeParams, N>& aparams) {
        this->set_many_attribute_params(aparams);
        return *this;
    }

    static void set_attribute_params(const AttributeParams& ap) {
        glVertexAttribPointer(
            ap.index, ap.size, ap.type, ap.normalized,
            ap.stride_bytes, reinterpret_cast<const void*>(ap.offset_bytes)
        );
    }

    void unbind() {
        glBindVertexArray(0u);
    }
};


class VAO : public VAOAllocator {
public:
    BoundVAO bind() {
        glBindVertexArray(id_);
        return {};
    }
};






class BoundVBO {
private:
    friend class VBO;
    BoundVBO() = default;

public:
    template<typename T>
    BoundVBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    template<size_t N>
    BoundVBO& associate_with(BoundVAO& vao,
                        const std::array<AttributeParams, N>& aparams) {
        vao.associate_with(*this, aparams);
        return *this;
    }

    void unbind() {
        glBindBuffer(GL_ARRAY_BUFFER, 0u);
    }
};


class VBO : public BufferAllocator {
public:
    BoundVBO bind() {
        glBindBuffer(GL_ARRAY_BUFFER, id_);
        return {};
    }
};






class BoundEBO {
private:
    friend class EBO;
    BoundEBO() = default;

public:
    template<typename T>
    BoundEBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    void unbind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
    }
};


class EBO : public BufferAllocator {
public:
    BoundEBO bind(BoundVAO& vao) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
        return {};
    }
};




class TextureData;


class BoundTextureHandle {
private:
    friend class TextureHandle;
    BoundTextureHandle() = default;

public:
    BoundTextureHandle& attach_data(const TextureData& tex_data,
        GLenum internal_format = GL_RGBA, GLenum format = GL_NONE);
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






} // namespace learn
