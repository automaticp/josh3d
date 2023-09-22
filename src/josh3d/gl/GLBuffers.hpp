#pragma once
#include "AndThen.hpp"
#include "AsSelf.hpp"
#include "GLMutability.hpp"
#include "GLScalars.hpp"
#include "RawGLHandles.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>


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
struct BindableBufferIndexed
    : BindableBuffer<MutT, CRTP, BoundT, TargetV>
{
    BoundIndexedT<MutT> bind_to_index(GLuint binding_index) const noexcept {
        gl::glBindBufferBase(
            TargetV, binding_index,
            static_cast<const CRTP<MutT>&>(*this).id()
        );
        return { binding_index };
    }

    BoundIndexedT<MutT> bind_range_to_index(
        GLintptr offset, GLsizeiptr size, GLuint index) const noexcept
    {
        gl::glBindBufferRange(
            TargetV, index,
            static_cast<const CRTP<MutT>&>(*this).id(),
            offset, size
        );
        return { index };
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
    void unbind() const noexcept { gl::glBindBufferBase(TargetV, index_, 0); }
    GLuint binding_index() const noexcept { return index_; }
};


template<GLenum TargetV>
struct BoundBufferBase {
    static void unbind() noexcept { gl::glBindBuffer(TargetV, 0); }
};




template<typename CRTP, GLenum TargetV>
struct BoundBufferCommonImpl
    : public  detail::AndThen<CRTP>
    , private detail::AsSelf<CRTP>
{
    template<typename T>
    CRTP& get_sub_data(GLsizeiptr size, GLsizeiptr offset, T* data) {
        gl::glGetBufferSubData(
            TargetV, offset * sizeof(T), size * sizeof(T), data
        );
        return this->as_self();
    }
};



template<typename CRTP, GLenum TargetV>
struct BoundBufferMutableImpl
    : public BoundBufferCommonImpl<CRTP, TargetV>
{
    template<typename T>
    CRTP& specify_data(GLsizeiptr size, const T* data, GLenum usage) {
        gl::glBufferData(
            TargetV, size * sizeof(T), data, usage
        );
        return this->as_self();
    }

    template<typename T>
    CRTP& sub_data(GLsizeiptr size, GLsizeiptr offset, const T* data) {
        gl::glBufferSubData(
            TargetV, offset * sizeof(T), size * sizeof(T), data
        );
        return this->as_self();
    }

};












template<template<typename> typename BoundTemplateCRTP, mutability_tag MutT>
struct BoundBufferImpl;


#define SPECIALIZE_IMPL(buf_name, target_enum)                            \
    template<>                                                            \
    struct BoundBufferImpl<Bound##buf_name, GLConst>                      \
        : BoundBufferCommonImpl<Bound##buf_name<GLConst>, target_enum>    \
        , BoundBufferBase<target_enum>                                    \
    {};                                                                   \
                                                                          \
    template<>                                                            \
    struct BoundBufferImpl<Bound##buf_name, GLMutable>                    \
        : BoundBufferMutableImpl<Bound##buf_name<GLMutable>, target_enum> \
        , BoundBufferBase<target_enum>                                    \
    {};


SPECIALIZE_IMPL(VBO,  gl::GL_ARRAY_BUFFER)
SPECIALIZE_IMPL(EBO,  gl::GL_ELEMENT_ARRAY_BUFFER)
SPECIALIZE_IMPL(UBO,  gl::GL_UNIFORM_BUFFER)
SPECIALIZE_IMPL(SSBO, gl::GL_SHADER_STORAGE_BUFFER)

#undef SPECIALIZE_IMPL




template<template<typename> typename BoundTemplateCRTP, mutability_tag MutT>
struct BoundBufferIndexedImpl;


#define SPECIALIZE_INDEXED_IMPL(buf_name, target_enum)                           \
    template<>                                                                   \
    struct BoundBufferIndexedImpl<BoundIndexed##buf_name, GLConst>               \
        : BoundBufferCommonImpl<BoundIndexed##buf_name<GLConst>, target_enum>    \
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


SPECIALIZE_INDEXED_IMPL(UBO,  gl::GL_UNIFORM_BUFFER)
SPECIALIZE_INDEXED_IMPL(SSBO, gl::GL_SHADER_STORAGE_BUFFER)

#undef SPECIALIZE_INDEXED_IMPL




} // namespace detail




#define GENERATE_BUFFER_CLASSES(buf_name, target_enum)             \
    template<mutability_tag MutT>                                  \
    class Bound##buf_name                                          \
        : public detail::BoundBufferImpl<Bound##buf_name, MutT>    \
    {                                                              \
    private:                                                       \
        friend detail::Bindable##buf_name<MutT>;                   \
        Bound##buf_name() = default;                               \
    };                                                             \
                                                                   \
    template<mutability_tag MutT>                                  \
    class BoundIndexed##buf_name                                   \
        : public detail::BoundBufferIndexedImpl<                   \
            BoundIndexed##buf_name, MutT>                          \
    {                                                              \
    private:                                                       \
        friend detail::Bindable##buf_name<MutT>;                   \
        using detail::BoundBufferIndexedImpl<                      \
            BoundIndexed##buf_name, MutT>::BoundBufferIndexedImpl; \
    };                                                             \
                                                                   \
    template<mutability_tag MutT>                                  \
    class Raw##buf_name                                            \
        : public RawBufferHandle<MutT>                             \
        , public detail::Bindable##buf_name<MutT>                  \
    {                                                              \
    public:                                                        \
        using RawBufferHandle<MutT>::RawBufferHandle;              \
    };                                                             \
                                                                   \
    static_assert(sizeof(Raw##buf_name<GLConst>));                 \
    static_assert(sizeof(Raw##buf_name<GLMutable>));



GENERATE_BUFFER_CLASSES(VBO,  gl::GL_ARRAY_BUFFER)
GENERATE_BUFFER_CLASSES(EBO,  gl::GL_ELEMENT_ARRAY_BUFFER)
GENERATE_BUFFER_CLASSES(UBO,  gl::GL_UNIFORM_BUFFER)
GENERATE_BUFFER_CLASSES(SSBO, gl::GL_SHADER_STORAGE_BUFFER)

#undef GENERATE_BUFFER_CLASSES


} // namespace josh
