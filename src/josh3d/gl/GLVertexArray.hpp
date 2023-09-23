#pragma once
#include "detail/AndThen.hpp"
#include "detail/AsSelf.hpp"
#include "GLMutability.hpp"
#include "GLScalars.hpp"
#include "RawGLHandles.hpp"
#include "VertexConcept.hpp" // IWYU pragma: keep (concepts)
#include <glbinding/gl/gl.h>


namespace josh {

template<mutability_tag MutT> class BoundVBO;

template<mutability_tag MutT> class BoundVAO;
template<mutability_tag MutT> class RawVAO;



namespace detail {

template<typename CRTP>
struct BoundVAOImplCommon
    : AndThen<CRTP>
    , AsSelf<CRTP>
{
    static void unbind() { gl::glBindVertexArray(0); }

    CRTP& draw_arrays(GLenum mode, GLint first, GLsizei count) {
        gl::glDrawArrays(mode, first, count);
        return this->as_self();
    }

    CRTP& draw_elements(GLenum mode, GLsizei count, GLenum type,
        const void* indices_buffer = nullptr)
    {
        gl::glDrawElements(mode, count, type, indices_buffer);
        return this->as_self();
    }

    CRTP& draw_arrays_instanced(GLenum mode, GLint first,
        GLsizei count, GLsizei instance_count)
    {
        gl::glDrawArraysInstanced(
            mode, first, count, instance_count
        );
        return this->as_self();
    }

    CRTP& draw_elements_instanced(
        GLenum mode, GLsizei elem_count, GLenum type,
        GLsizei instance_count,
        const void* indices_buffer = nullptr)
    {
        gl::glDrawElementsInstanced(
            mode, elem_count, type, indices_buffer, instance_count
        );
        return this->as_self();
    }

};



template<typename CRTP>
struct BoundVAOImplMutable
    : BoundVAOImplCommon<CRTP>
{

    CRTP& enable_array_access(GLuint attrib_index) {
        gl::glEnableVertexAttribArray(attrib_index);
        return this->as_self();
    }

    CRTP& disable_array_access(GLuint attrib_index) {
        gl::glDisableVertexAttribArray(attrib_index);
        return this->as_self();
    }

    template<vertex_attribute_container AttrsT>
    [[deprecated]] CRTP& set_many_attribute_params(
        const AttrsT& aparams)
    {
        return enable_many_attribute_params(aparams);
    }

    template<vertex_attribute_container AttrsT>
    CRTP& enable_many_attribute_params(
        const AttrsT& aparams)
    {
        for (const AttributeParams& ap : aparams) {
            set_attribute_params(ap);
            enable_array_access(ap.index);
        }
        return this->as_self();
    }


    // Use this overload when the type of VertexT is known.
    template<vertex VertexT, mutability_tag MutT>
    CRTP& associate_with(
        const BoundVBO<MutT>& vbo [[maybe_unused]])
    {
        return enable_many_attribute_params(VertexT::get_attributes());
    }


    // Use this overload when the layout specification is custom
    // and does not depend on the vertex type,
    // or the attributes have to be specified manually.
    template<vertex_attribute_container AttrsT, mutability_tag MutT>
    CRTP& associate_with(
        const BoundVBO<MutT>& vbo [[maybe_unused]],
        const AttrsT& aparams)
    {
        return enable_many_attribute_params(aparams);
    }


    CRTP& set_attribute_params(const AttributeParams& ap) {
        gl::glVertexAttribPointer(
            ap.index, ap.size, ap.type, ap.normalized,
            ap.stride_bytes,
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            reinterpret_cast<const void*>(ap.offset_bytes)
        );
        return this->as_self();
    }
};




template<typename MutT>
struct BoundVAOImpl;

template<> struct BoundVAOImpl<GLConst>
    : BoundVAOImplCommon<BoundVAO<GLConst>>
{};

template<> struct BoundVAOImpl<GLMutable>
    : BoundVAOImplMutable<BoundVAO<GLMutable>>
{};



} // namespace detail




template<mutability_tag MutT>
class BoundVAO
    : public detail::BoundVAOImpl<MutT>
{
private:
    friend RawVAO<MutT>;
    BoundVAO() = default;
};


template<mutability_tag MutT>
class RawVAO
    : public RawVertexArrayHandle<MutT>
    , public detail::ObjectHandleTypeInfo<RawVAO, MutT>
{
public:
    using RawVertexArrayHandle<MutT>::RawVertexArrayHandle;

    BoundVAO<MutT> bind() const {
        gl::glBindVertexArray(this->id());
        return {};
    }
};




} // namespace josh
