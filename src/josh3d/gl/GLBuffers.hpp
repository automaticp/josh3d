#pragma once
#include "EnumUtils.hpp"
#include "detail/AndThen.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "GLMutability.hpp"
#include "GLScalars.hpp"
#include "GLKindHandles.hpp"
#include "GLVertexArray.hpp"
#include "AttributeParams.hpp" // IWYU pragma: keep (concepts)
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
#include <cassert>
#include <cstddef>
#include <span>


namespace josh {


template<mutability_tag MutT> class BoundVAO;
template<mutability_tag MutT> class BoundVBO;
template<mutability_tag MutT> class BoundEBO;
template<mutability_tag MutT> class BoundUBO;
template<mutability_tag MutT> class BoundIndexedUBO;
template<mutability_tag MutT> class BoundSSBO;
template<mutability_tag MutT> class BoundIndexedSSBO;

template<mutability_tag MutT> class RawVAO;
template<mutability_tag MutT> class RawVBO;
template<mutability_tag MutT> class RawEBO;
template<mutability_tag MutT> class RawUBO;
template<mutability_tag MutT> class RawSSBO;






namespace detail {


template<
    mutability_tag MutT,
    template<typename> typename CRTP,
    template<typename> typename BoundT,
    GLenum TargetV
>
struct BindableBuffer {
    BoundT<MutT> bind() const noexcept {
        gl::glBindBuffer(
            TargetV, static_cast<const CRTP<MutT>&>(*this).id()
        );
        return {};
    }
};


template<
    mutability_tag MutT,
    template<typename> typename CRTP,
    template<typename> typename BoundT,
    template<typename> typename BoundIndexedT,
    GLenum TargetV
>
struct BindableBufferIndexed {
    BoundT<MutT> bind() const noexcept {
        gl::glBindBuffer(
            TargetV, static_cast<const CRTP<MutT>&>(*this).id()
        );
        return {};
    }

    BoundIndexedT<MutT> bind_to_index(GLuint binding_index) const noexcept {
        gl::glBindBufferBase(
            TargetV, binding_index,
            static_cast<const CRTP<MutT>&>(*this).id()
        );
        return { binding_index };
    }

    BoundIndexedT<MutT> bind_byte_range_to_index(
        GLintptr offset_bytes, GLsizeiptr size_bytes, GLuint index) const noexcept
    {
        gl::glBindBufferRange(
            TargetV, index,
            static_cast<const CRTP<MutT>&>(*this).id(),
            offset_bytes, size_bytes
        );
        return { index };
    }

    template<typename T>
    BoundIndexedT<MutT> bind_range_to_index(
        GLintptr elem_offset, GLsizeiptr elem_count, GLuint index) const noexcept
    {
        return bind_byte_range_to_index(
            elem_offset * sizeof(T), elem_count * sizeof(T), index
        );
    }

};


template<mutability_tag MutT> using BindableVBO =
    BindableBuffer<MutT, RawVBO, BoundVBO, gl::GL_ARRAY_BUFFER>;

template<mutability_tag MutT> using BindableEBO =
    BindableBuffer<MutT, RawEBO, BoundEBO, gl::GL_ELEMENT_ARRAY_BUFFER>;

template<mutability_tag MutT> using BindableUBO =
    BindableBufferIndexed<MutT, RawUBO, BoundUBO, BoundIndexedUBO, gl::GL_UNIFORM_BUFFER>;

template<mutability_tag MutT> using BindableSSBO =
    BindableBufferIndexed<MutT, RawSSBO, BoundSSBO, BoundIndexedSSBO, gl::GL_SHADER_STORAGE_BUFFER>;








template<GLenum TargetV>
struct BoundBufferIndexedBase {
private:
    GLuint index_;
public:
    BoundBufferIndexedBase(GLuint index) : index_{ index } {}
    static void unbind_at_index(GLuint index) noexcept { gl::glBindBufferBase(TargetV, index, 0); }
    void unbind() const noexcept { unbind_at_index(index_); }
    GLuint binding_index() const noexcept { return index_; }
};


template<GLenum TargetV>
struct BoundBufferBase {
    static void unbind() noexcept { gl::glBindBuffer(TargetV, 0); }
};



template<typename T>
std::span<T> map_buffer_range_impl(
    GLenum target, GLintptr elem_offset, GLsizeiptr elem_count,
    gl::MapBufferAccessMask access, gl::MapBufferAccessMask rw_bits)
{
    constexpr auto rw_maskout =
        gl::MapBufferAccessMask{
            ~to_underlying( // Bitfields in glbinding don't support `operator~`...
                gl::MapBufferAccessMask{ gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT }
            )
        };
    access = (access & rw_maskout) | rw_bits;
    void* buf =
        gl::glMapBufferRange(
            target, elem_offset * sizeof(T), elem_count * sizeof(T), access
        );
    return { static_cast<T*>(buf), elem_count };
}




template<typename CRTP, GLenum TargetV>
struct BoundBufferConstImpl
    : public AndThen<CRTP>
{
    template<typename T>
    CRTP& get_sub_data_into(std::span<T> dst_buf, GLsizeiptr offset = 0) {
        gl::glGetBufferSubData(
            TargetV, offset * sizeof(T), dst_buf.size_bytes(), dst_buf.data()
        );
        return static_cast<CRTP&>(*this);
    }

    GLsizeiptr get_size_bytes() {
        GLint64 size;
        gl::glGetBufferParameteri64v(TargetV, gl::GL_BUFFER_SIZE, &size);
        return size;
    }

    template<typename T>
    GLsizeiptr get_size() {
        return get_size_bytes() / sizeof(T);
    }

    // Maps a subrange of the buffer storage for read-only operations.
    // The Read/Write bits in `access` are ignored by this call.
    template<typename T>
    [[nodiscard]] std::span<const T> map_for_read(
        GLintptr elem_offset, GLsizeiptr elem_count, gl::MapBufferAccessMask access = {})
    {
        return map_buffer_range_impl<T>(
            TargetV, elem_offset, elem_count,
            access, gl::GL_MAP_READ_BIT
        );
    }

    // Maps a the entire buffer storage for read-only operations.
    // The Read/Write bits in `access` are ignored by this call.
    template<typename T>
    [[nodiscard]] std::span<const T> map_for_read(gl::MapBufferAccessMask access = {}) {
        return map_for_read<T>(0, get_size<T>(), access);
    }

    CRTP& unmap_current() {
        // This can fail according to spec, but does it ever?
        //
        // Quote:
        //     "glUnmapBuffer returns GL_TRUE unless the data store contents
        //     have become corrupt during the time the data store was mapped.
        //     This can occur for system-specific reasons that affect
        //     the availability of graphics memory, such as screen mode changes.
        //     In such situations, GL_FALSE is returned and the data store contents
        //     are undefined. An application must detect this rare condition
        //     and reinitialize the data store."
        GLboolean unmapping_succeded = gl::glUnmapBuffer(TargetV);
        assert(unmapping_succeded && "It's your unlucky day!");
        return static_cast<CRTP&>(*this);
    }
};



template<typename CRTP, GLenum TargetV>
struct BoundBufferMutableImpl
    : public BoundBufferConstImpl<CRTP, TargetV>
{
    template<typename T>
    CRTP& specify_data(std::span<const T> src_buf, GLenum usage) {
        gl::glBufferData(
            TargetV, src_buf.size_bytes(), src_buf.data(), usage
        );
        return static_cast<CRTP&>(*this);
    }

    template<typename T>
    CRTP& allocate_data(GLsizeiptr size, GLenum usage) {
        gl::glBufferData(
            TargetV, size * sizeof(T), nullptr, usage
        );
        return static_cast<CRTP&>(*this);
    }

    template<typename T>
    CRTP& sub_data(std::span<const T> src_buf, GLsizeiptr offset = 0) {
        gl::glBufferSubData(
            TargetV, offset * sizeof(T), src_buf.size_bytes(), src_buf.data()
        );
        return static_cast<CRTP&>(*this);
    }

    // Maps a subrange of the buffer storage for write-only operations.
    // The Read/Write bits in `access` are ignored by this call.
    template<typename T>
    [[nodiscard]] std::span<T> map_for_write(
        GLintptr elem_offset, GLsizeiptr elem_count, gl::MapBufferAccessMask access = {})
    {
        return map_buffer_range_impl<T>(
            TargetV, elem_offset, elem_count,
            access, gl::GL_MAP_WRITE_BIT
        );
    }

    // Maps the whole buffer storage for write-only operations.
    // The Read/Write bits in `access` are ignored by this call.
    template<typename T>
    [[nodiscard]] std::span<T> map_for_write(gl::MapBufferAccessMask access = {}) {
        return map_for_write<T>(0, this->template get_size<T>(), access);
    }

    // Maps a subrange of the buffer storage for read and write operations.
    // The Read/Write bits in `access` are ignored by this call.
    template<typename T>
    [[nodiscard]] std::span<T> map_for_readwrite(
        GLintptr elem_offset, GLsizeiptr elem_count, gl::MapBufferAccessMask access = {})
    {
        return map_buffer_range_impl<T>(
            TargetV, elem_offset, elem_count,
            access, (gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT)
        );
    }

    // Maps the whole buffer storage for read and write operations.
    // The Read/Write bits in `access` are ignored by this call.
    template<typename T>
    [[nodiscard]] std::span<T> map_for_readwrite(gl::MapBufferAccessMask access = {}) {
        return map_for_readwrite<T>(0, this->template get_size<T>(), access);
    }


    // The buffer object must previously have been mapped with the `GL_MAP_FLUSH_EXPLICIT_BIT` flag.
    template<typename T>
    CRTP& flush_mapped_range(GLintptr elem_offset, GLsizeiptr elem_count) {
        gl::glFlushMappedBufferRange(
            TargetV, elem_offset * sizeof(T), elem_count * sizeof(T)
        );
        return static_cast<CRTP&>(*this);
    }

};




template<typename CRTP>
struct BoundBufferVBOAssociate {

    template<vertex VertexT>
    CRTP& associate_with(BoundVAO<GLMutable>& bvao) {
        bvao.associate_with<VertexT>(static_cast<CRTP&>(*this));
        return static_cast<CRTP&>(*this);
    }

    template<vertex VertexT>
    CRTP& associate_with(BoundVAO<GLMutable>&& bvao) {
        return associate_with<VertexT>(bvao);
    }

    template<vertex_attribute_container AttrsT>
    CRTP& associate_with(BoundVAO<GLMutable>& bvao, const AttrsT& aparams) {
        bvao.associate_with(static_cast<CRTP&>(*this), aparams);
        return static_cast<CRTP&>(*this);
    }

    template<vertex_attribute_container AttrsT>
    CRTP& associate_with(BoundVAO<GLMutable>&& bvao, const AttrsT& aparams) {
        return associate_with(bvao, aparams);
    }
};




template<template<typename> typename BoundTemplateCRTP, mutability_tag MutT>
struct BoundBufferSpecificImpl {};

template<mutability_tag MutT>
struct BoundBufferSpecificImpl<BoundVBO, MutT>
    : BoundBufferVBOAssociate<BoundVBO<MutT>>
{};





template<template<typename> typename BoundTemplateCRTP, mutability_tag MutT>
struct BoundBufferImpl;


#define JOSH3D_SPECIALIZE_IMPL(buf_name, target_enum)                     \
    template<>                                                            \
    struct BoundBufferImpl<Bound##buf_name, GLConst>                      \
        : BoundBufferConstImpl<Bound##buf_name<GLConst>, target_enum>     \
        , BoundBufferSpecificImpl<Bound##buf_name, GLConst>               \
        , BoundBufferBase<target_enum>                                    \
    {};                                                                   \
                                                                          \
    template<>                                                            \
    struct BoundBufferImpl<Bound##buf_name, GLMutable>                    \
        : BoundBufferMutableImpl<Bound##buf_name<GLMutable>, target_enum> \
        , BoundBufferSpecificImpl<Bound##buf_name, GLMutable>             \
        , BoundBufferBase<target_enum>                                    \
    {};


JOSH3D_SPECIALIZE_IMPL(VBO,  gl::GL_ARRAY_BUFFER)
JOSH3D_SPECIALIZE_IMPL(EBO,  gl::GL_ELEMENT_ARRAY_BUFFER)
JOSH3D_SPECIALIZE_IMPL(UBO,  gl::GL_UNIFORM_BUFFER)
JOSH3D_SPECIALIZE_IMPL(SSBO, gl::GL_SHADER_STORAGE_BUFFER)

#undef JOSH3D_SPECIALIZE_IMPL




template<template<typename> typename BoundTemplateCRTP, mutability_tag MutT>
struct BoundBufferIndexedImpl;


#define JOSH3D_SPECIALIZE_INDEXED_IMPL(buf_name, target_enum)                    \
    template<>                                                                   \
    struct BoundBufferIndexedImpl<BoundIndexed##buf_name, GLConst>               \
        : BoundBufferConstImpl<BoundIndexed##buf_name<GLConst>, target_enum>     \
        , BoundBufferIndexedBase<target_enum>                                    \
    {                                                                            \
        using BoundBufferIndexedBase<target_enum>::BoundBufferIndexedBase;       \
    };                                                                           \
                                                                                 \
    template<>                                                                   \
    struct BoundBufferIndexedImpl<BoundIndexed##buf_name, GLMutable>             \
        : BoundBufferMutableImpl<BoundIndexed##buf_name<GLMutable>, target_enum> \
        , BoundBufferIndexedBase<target_enum>                                    \
    {                                                                            \
        using BoundBufferIndexedBase<target_enum>::BoundBufferIndexedBase;       \
    };


JOSH3D_SPECIALIZE_INDEXED_IMPL(UBO,  gl::GL_UNIFORM_BUFFER)
JOSH3D_SPECIALIZE_INDEXED_IMPL(SSBO, gl::GL_SHADER_STORAGE_BUFFER)

#undef JOSH3D_SPECIALIZE_INDEXED_IMPL




} // namespace detail




#define JOSH3D_GENERATE_BUFFER_CLASSES(buf_name, target_enum)                 \
    template<mutability_tag MutT>                                             \
    class Bound##buf_name                                                     \
        : public detail::BoundBufferImpl<Bound##buf_name, MutT>               \
    {                                                                         \
    private:                                                                  \
        friend detail::Bindable##buf_name<MutT>;                              \
        Bound##buf_name() = default;                                          \
    };                                                                        \
                                                                              \
    template<mutability_tag MutT>                                             \
    class BoundIndexed##buf_name                                              \
        : public detail::BoundBufferIndexedImpl<                              \
            BoundIndexed##buf_name, MutT>                                     \
    {                                                                         \
    private:                                                                  \
        friend detail::Bindable##buf_name<MutT>;                              \
        using detail::BoundBufferIndexedImpl<                                 \
            BoundIndexed##buf_name, MutT>::BoundBufferIndexedImpl;            \
    };                                                                        \
                                                                              \
    template<mutability_tag MutT>                                             \
    class Raw##buf_name                                                       \
        : public RawBufferHandle<MutT>                                        \
        , public detail::Bindable##buf_name<MutT>                             \
        , public detail::ObjectHandleTypeInfo<Raw##buf_name, MutT>            \
    {                                                                         \
    public:                                                                   \
        JOSH3D_MAGIC_CONSTRUCTORS(Raw##buf_name, MutT, RawBufferHandle<MutT>) \
    };                                                                        \
                                                                              \
    static_assert(sizeof(Raw##buf_name<GLConst>));                            \
    static_assert(sizeof(Raw##buf_name<GLMutable>));



JOSH3D_GENERATE_BUFFER_CLASSES(VBO,  gl::GL_ARRAY_BUFFER)
JOSH3D_GENERATE_BUFFER_CLASSES(EBO,  gl::GL_ELEMENT_ARRAY_BUFFER)
JOSH3D_GENERATE_BUFFER_CLASSES(UBO,  gl::GL_UNIFORM_BUFFER)
JOSH3D_GENERATE_BUFFER_CLASSES(SSBO, gl::GL_SHADER_STORAGE_BUFFER)

#undef JOSH3D_GENERATE_BUFFER_CLASSES


} // namespace josh
