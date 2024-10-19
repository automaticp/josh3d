#pragma once
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLBuffers.hpp"
#include "GLAttributeTraits.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include "detail/StaticAssertFalseMacro.hpp"
#include "detail/StrongScalar.hpp"
#include <concepts>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>



namespace josh {


JOSH3D_DEFINE_STRONG_SCALAR(AttributeIndex,   GLuint);
JOSH3D_DEFINE_STRONG_SCALAR(VertexBufferSlot, GLuint);

JOSH3D_DEFINE_STRONG_SCALAR(StrideBytes, GLsizei);




namespace detail {


template<typename CRTP>
struct VertexArrayDSAInterface_BufferAttachments {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    template<of_kind<GLKind::Buffer> BufferT>
    void attach_element_buffer(
        const BufferT& element_buffer) const noexcept
            requires mt::is_mutable
    {

        gl::glVertexArrayElementBuffer(self_id(), decay_to_raw(element_buffer).id());
    }

    void detach_element_buffer() const noexcept
        requires mt::is_mutable
    {
        gl::glVertexArrayElementBuffer(self_id(), 0);
    }


    template<of_kind<GLKind::Buffer> BufferT>
    void attach_vertex_buffer(
        VertexBufferSlot buffer_slot,
        const BufferT&   buffer,
        OffsetBytes      offset_bytes,
        StrideBytes      stride_bytes) const noexcept
            requires mt::is_mutable
    {
        /*
        "[4.6, 10.3] If buffer is not the name of an existing buffer object, the GL first creates a new
        state vector, initialized with a zero-sized memory buffer and comprising all the
        state and with the same initial values listed in table 6.2, just as for BindBuffer.
        buffer is then attached to the specified bindingindex of the vertex array object."

        WHAT THE FUCK?

        At the same time:

        "An INVALID_OPERATION error is generated if buffer is not zero or a name
        returned from a previous call to GenBuffers or CreateBuffers, or if such a
        name has since been deleted with DeleteBuffers."

        Maybe that's a mishap in the spec, idk.

        I'll check just in case anyway.
        */
        assert(gl::glIsBuffer(decay_to_raw(buffer).id()));
        gl::glVertexArrayVertexBuffer(
            self_id(),
            buffer_slot,
            decay_to_raw(buffer).id(),
            offset_bytes,
            stride_bytes
        );
    }

    void detach_vertex_buffer(
        VertexBufferSlot buffer_slot) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayVertexBuffer(
            self_id(), buffer_slot,
            0, 0, 0
        );
    }


    void attach_vertex_buffers(
        VertexBufferSlot          first_buffer_slot,
        std::span<const GLuint>   buffer_ids,
        std::span<const GLintptr> buffer_offsets_bytes,
        std::span<const GLsizei>  buffer_strides_bytes) const noexcept
            requires mt::is_mutable
    {
        assert(buffer_ids.size() == buffer_offsets_bytes.size());
        assert(buffer_ids.size() == buffer_strides_bytes.size());
        gl::glVertexArrayVertexBuffers(
            self_id(), first_buffer_slot,
            buffer_ids.size(),
            buffer_ids.data(),
            buffer_offsets_bytes.data(),
            buffer_strides_bytes.data()
        );
    }

    void detach_vertex_buffers(
        VertexBufferSlot first_buffer_slot,
        GLsizei          num_slots) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayVertexBuffers(
            self_id(), first_buffer_slot,
            num_slots, nullptr, nullptr, nullptr
        );
    }

};




template<typename CRTP>
struct VertexArrayDSAInterface_AttributeSpecification {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    // TODO: There are some rules on packed types that restrict possible size values.

    // vec
    void specify_float_attribute(
        AttributeIndex      attrib_index,
        AttributeTypeF      type,
        AttributeComponents components,
        OffsetBytes         offset_bytes = OffsetBytes{ 0 }) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayAttribFormat(
            self_id(), attrib_index,
            enum_cast<GLint>(components),
            enum_cast<GLenum>(type),
            gl::GL_FALSE, offset_bytes
        );
    }

    // vec
    void specify_float_attribute_normalized(
        AttributeIndex      attrib_index,
        AttributeTypeNorm   type,
        AttributeComponents components,
        OffsetBytes         offset_bytes = OffsetBytes{ 0 }) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayAttribFormat(
            self_id(), attrib_index,
            enum_cast<GLint>(components),
            enum_cast<GLenum>(type),
            gl::GL_TRUE, offset_bytes
        );
    }

    // vec
    void specify_float_attribute_normalized_bgra(
        AttributeIndex          attrib_index,
        AttributeTypeBGRA       type,
        AttributeComponentsBGRA components,
        OffsetBytes             offset_bytes = OffsetBytes{ 0 }) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayAttribFormat(
            self_id(), attrib_index,
            enum_cast<GLint>(components),
            enum_cast<GLenum>(type),
            gl::GL_TRUE, offset_bytes
        );
    }


    // vec
    void specify_float_attribute_cast_to_float(
        AttributeIndex      attrib_index,
        AttributeTypeNorm   type,
        AttributeComponents components,
        OffsetBytes         offset_bytes = OffsetBytes{ 0 }) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayAttribFormat(
            self_id(), attrib_index,
            enum_cast<GLint>(components),
            enum_cast<GLenum>(type),
            gl::GL_FALSE, offset_bytes
        );
    }

    // ivec, uvec
    void specify_integer_attribute(
        AttributeIndex      attrib_index,
        AttributeTypeI      type,
        AttributeComponents components,
        OffsetBytes         offset_bytes = OffsetBytes{ 0 }) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayAttribIFormat(
            self_id(), attrib_index,
            enum_cast<GLint>(components),
            enum_cast<GLenum>(type),
            offset_bytes
        );
    }

    // dvec
    void specify_double_attribute(
        AttributeIndex      attrib_index,
        AttributeTypeD      type,
        AttributeComponents components,
        OffsetBytes         offset_bytes = OffsetBytes{ 0 }) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayAttribLFormat(
            self_id(),  attrib_index,
            enum_cast<GLint>(components),
            enum_cast<GLenum>(type),
            offset_bytes
        );
    }




    // Sources attribute specification from the `attribute_traits<VertexT>::specs`.
    //
    // This creates no association with any particular vertex buffer of buffer slot.
    //
    // Returns the number of specified attributes starting from `first_attrib_index`.
    // This will always be equal to `std::tuple_size_v<decltype(attribute_traits<VertexT>::specs)>`
    template<specializes_attribute_traits VertexT>
    GLsizeiptr specify_custom_attributes(
        AttributeIndex first_attrib_index) const noexcept
            requires mt::is_mutable
    {
        using tr = attribute_traits<VertexT>;
        constexpr size_t num_attribs = std::tuple_size_v<decltype(tr::specs)>;

        auto apply_attrib = [this]<typename AttribSpecT>(
            AttributeIndex index, const AttribSpecT& spec)
        {
            if constexpr      (std::same_as<AttribSpecT, AttributeSpecF>)     { specify_float_attribute                (index, spec.type, spec.components, spec.offset_bytes); }
            else if constexpr (std::same_as<AttribSpecT, AttributeSpecNorm>)  { specify_float_attribute_normalized     (index, spec.type, spec.components, spec.offset_bytes); }
            else if constexpr (std::same_as<AttribSpecT, AttributeSpecBGRA>)  { specify_float_attribute_normalized_bgra(index, spec.type, spec.components, spec.offset_bytes); }
            else if constexpr (std::same_as<AttribSpecT, AttributeSpecFCast>) { specify_float_attribute_cast_to_float  (index, spec.type, spec.components, spec.offset_bytes); }
            else if constexpr (std::same_as<AttribSpecT, AttributeSpecI>)     { specify_integer_attribute              (index, spec.type, spec.components, spec.offset_bytes); }
            else if constexpr (std::same_as<AttribSpecT, AttributeSpecD>)     { specify_double_attribute               (index, spec.type, spec.components, spec.offset_bytes); }
            else { JOSH3D_STATIC_ASSERT_FALSE_MSG(AttribSpecT, "Unsupported AttributeSpec Type"); }
        };

        const size_t first_index = first_attrib_index;

        [&]<size_t ...Idx>(std::index_sequence<Idx...>) {
            (apply_attrib(AttributeIndex{ GLuint(first_index + Idx) }, std::get<Idx>(tr::specs)), ...);
        }(std::make_index_sequence<num_attribs>());

        return num_attribs;
    }


};








template<typename CRTP>
struct VertexArrayDSAInterface_BindingToAttributeAssociation {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    void associate_attribute_with_buffer_slot(
        AttributeIndex   attrib_index,
        VertexBufferSlot buffer_slot) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayAttribBinding(self_id(), attrib_index, buffer_slot);
    }

};




template<typename CRTP>
struct VertexArrayDSAInterface_EnableDisableAttributeAccess {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    // "[4.6, 10.3.9] If any enabled array's buffer binding is zero when DrawArrays or one of the
    // other drawing commands defined in section 10.4 is called, the result is undefined."
    void enable_attribute(AttributeIndex attrib_index) const noexcept
        requires mt::is_mutable
    {
        gl::glEnableVertexArrayAttrib(self_id(), attrib_index);
    }

    void disable_attribute(AttributeIndex attrib_index) const noexcept
        requires mt::is_mutable
    {
        gl::glDisableVertexArrayAttrib(self_id(), attrib_index);
    }

};












template<typename CRTP>
struct VertexArrayDSAInterface_Queries {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
private:
    auto get_integer(GLenum pname) const noexcept {
        GLint result;
        gl::glGetVertexArrayiv(self_id(), pname, &result);
        return result;
    }
    auto get_boolean_indexed(GLenum pname, GLuint index) const noexcept {
        GLboolean result;
        gl::glGetVertexArrayIndexediv(self_id(), index, pname, &result);
        return result;
    }
    auto get_integer_indexed(GLenum pname, GLuint index) const noexcept {
        GLint result;
        gl::glGetVertexArrayIndexediv(self_id(), index, pname, &result);
        return result;
    }
    auto get_enum_indexed(GLenum pname, GLuint index) const noexcept {
        GLenum result;
        gl::glGetVertexArrayIndexediv(self_id(), index, pname, &result);
        return result;
    }
    auto get_integer64_indexed(GLenum pname, GLuint index) const noexcept {
        GLint64 result;
        gl::glGetVertexArrayIndexed64iv(self_id(), index, pname, &result);
        return result;
    }
public:

    GLuint get_attached_element_buffer_id() const noexcept {
        return get_integer(gl::GL_ELEMENT_ARRAY_BUFFER_BINDING);
    }

    OffsetBytes get_attached_vertex_buffer_offset_bytes(VertexBufferSlot buffer_slot) const noexcept {
        return get_integer64_indexed(gl::GL_VERTEX_BINDING_OFFSET, buffer_slot);
    }

    StrideBytes get_attached_vertex_buffer_stride_bytes(VertexBufferSlot buffer_slot) const noexcept {
        return get_integer_indexed(gl::GL_VERTEX_BINDING_STRIDE, buffer_slot);
    }

    GLuint get_attached_vertex_buffer_id(VertexBufferSlot buffer_slot) const noexcept {
        return get_integer_indexed(gl::GL_VERTEX_BINDING_BUFFER, buffer_slot);
    }

    GLuint get_buffer_slot_divisor(VertexBufferSlot buffer_slot) const noexcept {
        return get_integer_indexed(gl::GL_VERTEX_BINDING_DIVISOR, buffer_slot);
    }


    bool is_attribute_enabled(AttributeIndex attrib_index) const noexcept {
        return get_boolean_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_ENABLED, attrib_index);
    }

    auto get_attribute_components(AttributeIndex attrib_index) const noexcept
        -> AttributeComponentsAll
    {
        return AttributeComponentsAll{
            get_integer_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_SIZE, attrib_index)
        };
    }

    auto get_attribute_type(AttributeIndex attrib_index) const noexcept
        -> AttributeType
    {
        return enum_cast<AttributeType>(get_enum_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_TYPE, attrib_index));
    }

    bool is_attribute_normalized(AttributeIndex attrib_index) const noexcept {
        return get_boolean_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, attrib_index);
    }

    bool is_attribute_integer(AttributeIndex attrib_index) const noexcept {
        return get_boolean_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_INTEGER, attrib_index);
    }

    bool is_attribute_double(AttributeIndex attrib_index) const noexcept {
        return get_boolean_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_LONG, attrib_index);
    }


    StrideBytes get_associated_slot_stride_bytes(AttributeIndex attrib_index) const noexcept {
        return StrideBytes{ get_integer_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_STRIDE, attrib_index) };
    }

    GLuint get_associated_slot_divisor(AttributeIndex attrib_index) const noexcept {
        return get_integer_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_DIVISOR, attrib_index);
    }

    OffsetBytes get_associated_slot_offset_bytes(AttributeIndex attrib_index) const noexcept {
        return OffsetBytes{ get_integer_indexed(gl::GL_VERTEX_ATTRIB_RELATIVE_OFFSET, attrib_index) };
    }

    auto get_associated_slot(AttributeIndex attrib_index) const noexcept
        -> VertexBufferSlot
    {
        return VertexBufferSlot{
            get_integer_indexed(gl::GL_VERTEX_ATTRIB_BINDING, attrib_index)
        };
    }

    // TODO: I am SOOOOO unsure about this. Does this return the NAME of the buffer?
    GLuint get_associated_slot_buffer_id(AttributeIndex attrib_index) const noexcept {
        return get_integer_indexed(gl::GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, attrib_index);
    }

};








template<typename CRTP>
struct VertexArrayDSAInterface_Bind {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:
    [[nodiscard("BindTokens have to be provided to an API call that expects bound state.")]]
    auto bind() const noexcept
        -> BindToken<Binding::VertexArray>
    {
        gl::glBindVertexArray(self_id());
        return { self_id() };
    }
};












template<typename CRTP>
struct VertexArrayDSAInterface
    : VertexArrayDSAInterface_BufferAttachments<CRTP>
    , VertexArrayDSAInterface_AttributeSpecification<CRTP>
    , VertexArrayDSAInterface_BindingToAttributeAssociation<CRTP>
    , VertexArrayDSAInterface_EnableDisableAttributeAccess<CRTP>
    , VertexArrayDSAInterface_Queries<CRTP>
    , VertexArrayDSAInterface_Bind<CRTP>
{
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    void set_buffer_slot_divisor(
        VertexBufferSlot buffer_slot,
        GLuint           divisor) const noexcept
            requires mt::is_mutable
    {
        gl::glVertexArrayBindingDivisor(self_id(), buffer_slot, divisor);
    }


};




} // namespace detail





template<mutability_tag MutT = GLMutable>
class RawVertexArray
    : public detail::RawGLHandle<MutT>
    , public detail::VertexArrayDSAInterface<RawVertexArray<MutT>>
{
public:
    static constexpr GLKind kind_type = GLKind::VertexArray;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawVertexArray, mutability_traits<RawVertexArray>, detail::RawGLHandle<MutT>)
};




} // namespace josh
