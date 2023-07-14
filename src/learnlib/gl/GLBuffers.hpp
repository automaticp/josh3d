#pragma once
#include "GLObjectHandles.hpp"
#include "GLScalars.hpp"
#include "AndThen.hpp"
#include "VertexConcept.hpp"
#include <glbinding/gl/gl.h>
#include <array>




namespace josh {


/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/


namespace detail {


template<typename CRTP>
class VAODraw {
public:
    CRTP& draw_arrays(GLenum mode, GLint first, GLsizei count) {
        gl::glDrawArrays(mode, first, count);
        return as_self();
    }

    CRTP& draw_elements(GLenum mode, GLsizei count, GLenum type,
        const void* indices_buffer = nullptr)
    {
        glDrawElements(mode, count, type, indices_buffer);
        return as_self();
    }

    CRTP& draw_arrays_instanced(GLenum mode, GLint first,
        GLsizei count, GLsizei instance_count)
    {
        glDrawArraysInstanced(mode, first, count, instance_count);
        return as_self();
    }

    CRTP& draw_elements_instanced(GLenum mode,
        GLsizei elem_count, GLenum type, GLsizei instance_count,
        const void* indices_buffer = nullptr)
    {
        glDrawElementsInstanced(mode, elem_count, type, indices_buffer, instance_count);
        return as_self();
    }

private:
    CRTP& as_self() noexcept { return static_cast<CRTP&>(*this); }
};


} // namespace detail



namespace leaksgl {

using namespace gl;






class BoundConstVAO
    : public detail::AndThen<BoundConstVAO>
    , public detail::VAODraw<BoundConstVAO>
{
private:
    friend class VAO;
    BoundConstVAO() = default;

public:
    static void unbind() {
        glBindVertexArray(0u);
    }
};




class BoundVAO
    : public detail::AndThen<BoundVAO>
    , public detail::VAODraw<BoundVAO>
{
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

    template<vertex_attribute_container AttrsT>
    BoundVAO& set_many_attribute_params(
        const AttrsT& aparams)
    {
        for (const AttributeParams& ap : aparams) {
            set_attribute_params(ap);
            this->enable_array_access(ap.index);
        }
        return *this;
    }


    // Use this overload when the type of VertexT is known.
    template<vertex VertexT>
    BoundVAO& associate_with(const class BoundVBO& vbo) {
        this->set_many_attribute_params(VertexT::get_attributes());
        return *this;
    }


    // Use this overload when the layout specification is custom
    // and does not depend on the vertex type,
    // or the attributes have to be specified manually.
    template<vertex_attribute_container AttrsT>
    BoundVAO& associate_with(const class BoundVBO& vbo,
        const AttrsT& aparams)
    {
        this->set_many_attribute_params(aparams);
        return *this;
    }


    static void set_attribute_params(const AttributeParams& ap) {
        glVertexAttribPointer(
            ap.index, ap.size, ap.type, ap.normalized,
            ap.stride_bytes, reinterpret_cast<const void*>(ap.offset_bytes)
        );
    }

    static void unbind() {
        glBindVertexArray(0u);
    }
};




class VAO : public VAOHandle {
public:
    BoundVAO bind() {
        glBindVertexArray(id_);
        return {};
    }

    BoundConstVAO bind() const {
        glBindVertexArray(id_);
        return {};
    }
};











class BoundAbstractBuffer : public detail::AndThen<BoundAbstractBuffer> {
private:
    GLenum type_;

    friend class AbstractBuffer;
    BoundAbstractBuffer(GLenum type) : type_{ type } {}

public:
    template<typename T>
    BoundAbstractBuffer& attach_data(size_t size, const T* data,
        GLenum usage)
    {
        glBufferData(
            type_,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    template<typename T>
    BoundAbstractBuffer& sub_data(size_t size,
        size_t offset, const T* data)
    {
        glBufferSubData(
            type_,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data)
        );
        return *this;
    }

    template<typename T>
    BoundAbstractBuffer& get_sub_data(size_t size,
        size_t offset, T* data)
    {
        glGetBufferSubData(
            type_,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<void*>(data)
        );
        return *this;
    }



    void unbind() {
        glBindBuffer(type_, 0);
    }
};


class AbstractBuffer : public BufferHandle {
public:
    BoundAbstractBuffer bind_as(GLenum type) {
        glBindBuffer(type, id_);
        return { type };
    }

    static void unbind_as(GLenum type) {
        glBindBuffer(type, 0);
    }
};











class BoundSSBO : public detail::AndThen<BoundSSBO> {
private:
    friend class SSBO;
    BoundSSBO() = default;

public:
    template<typename T>
    BoundSSBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    template<typename T>
    BoundSSBO& sub_data(size_t size, size_t offset, const T* data) {
        glBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data)
        );
        return *this;
    }

    template<typename T>
    BoundSSBO& get_sub_data(size_t size,
        size_t offset, T* data)
    {
        glGetBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<void*>(data)
        );
        return *this;
    }


    static void unbind() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0u);
    }
};


class SSBO : public BufferHandle {
public:
    BoundSSBO bind() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
        return {};
    }

    BoundSSBO bind_to(GLuint binding_index) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_index, id_);
        return {};
    }
};











class BoundVBO : public detail::AndThen<BoundVBO>{
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


    template<vertex_attribute_container AttrsT>
    BoundVBO& associate_with(BoundVAO& vao,
        const AttrsT& aparams)
    {
        vao.associate_with(*this, aparams);
        return *this;
    }


    template<vertex_attribute_container AttrsT>
    BoundVBO& associate_with(BoundVAO&& vao,
        const AttrsT& aparams)
    {
        return this->associate_with(vao, aparams);
    }


    template<vertex VertexT>
    BoundVBO& associate_with(BoundVAO& vao) {
        vao.associate_with<VertexT>(*this);
        return *this;
    }


    template<vertex VertexT>
    BoundVBO& associate_with(BoundVAO&& vao) {
        return this->associate_with<VertexT>(vao);
    }



    static void unbind() {
        glBindBuffer(GL_ARRAY_BUFFER, 0u);
    }
};


class VBO : public BufferHandle {
public:
    BoundVBO bind() {
        glBindBuffer(GL_ARRAY_BUFFER, id_);
        return {};
    }
};











class BoundEBO : public detail::AndThen<BoundEBO> {
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

    static void unbind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
    }
};


class EBO : public BufferHandle {
public:
    BoundEBO bind(BoundVAO& vao) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
        return {};
    }
};







} // namespace leaksgl


using leaksgl::BoundAbstractBuffer, leaksgl::AbstractBuffer;
using leaksgl::BoundVAO, leaksgl::VAO;
using leaksgl::BoundVBO, leaksgl::VBO;
using leaksgl::BoundSSBO, leaksgl::SSBO;
using leaksgl::BoundEBO, leaksgl::EBO;


} // namespace josh
